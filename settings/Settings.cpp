#include "Settings.h"
#include "ui_Settings.h"

#include "movies/FilesWidget.h"
#include "main/MainWindow.h"
#include "globals/Manager.h"
#include "main/MessageBox.h"
#include "tvShows/TvShowFilesWidget.h"

Settings *Settings::m_instance;

/**
 * @brief Settings::Settings
 * @param parent
 */
Settings::Settings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);

    m_instance = this;

    ui->movieDirs->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    ui->movieDirs->horizontalHeaderItem(2)->setToolTip(tr("Movies are in separate folders"));
    ui->movieDirs->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    ui->movieDirs->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    ui->tvShowDirs->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    ui->tvShowDirs->horizontalHeader()->setResizeMode(QHeaderView::Stretch);

    int scraperCounter = 0;
    foreach (ScraperInterface *scraper, Manager::instance()->scrapers()) {
        if (scraper->hasSettings()) {
            if (scraperCounter++ > 0) {
                QFrame *line = new QFrame(ui->groupBox_2);
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                ui->verticalLayoutScrapers->addWidget(line);
            }
            QWidget *scraperSettings = scraper->settingsWidget();
            scraperSettings->setParent(ui->groupBox_2);
            ui->verticalLayoutScrapers->addWidget(new QLabel(scraper->name(), ui->groupBox_2));
            ui->verticalLayoutScrapers->addWidget(scraperSettings);
        }
    }
    foreach (TvScraperInterface *scraper, Manager::instance()->tvScrapers()) {
        if (scraper->hasSettings()) {
            if (scraperCounter++ > 0) {
                QFrame *line = new QFrame(ui->groupBox_2);
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                ui->verticalLayoutScrapers->addWidget(line);
            }
            QWidget *scraperSettings = scraper->settingsWidget();
            scraperSettings->setParent(ui->groupBox_2);
            ui->verticalLayoutScrapers->addWidget(new QLabel(scraper->name(), ui->groupBox_2));
            ui->verticalLayoutScrapers->addWidget(scraperSettings);
        }
    }

    // Setup file dialogs
    m_logFileDialog = new QFileDialog(this, tr("Logfile"), QDir::homePath(), tr("Logfiles (*.log *.txt)"));
    m_logFileDialog->setFileMode(QFileDialog::AnyFile);
    m_movieDirDialog = new QFileDialog(this, tr("Choose a directory containing your movies"), QDir::homePath());
    m_movieDirDialog->setFileMode(QFileDialog::Directory);
    m_movieDirDialog->setOption(QFileDialog::ShowDirsOnly, true);
    m_tvShowDirDialog = new QFileDialog(this, tr("Choose a directory containing your TV shows"), QDir::homePath());
    m_tvShowDirDialog->setFileMode(QFileDialog::Directory);
    m_tvShowDirDialog->setOption(QFileDialog::ShowDirsOnly, true);
    m_xbmcThumbnailDirDialog = new QFileDialog(this, tr("Choose a directory containing your Thumbnails"), QDir::homePath());;
    m_xbmcThumbnailDirDialog->setOption(QFileDialog::ShowDirsOnly, true);
    m_xbmcSqliteDatabaseDialog = new QFileDialog(this, tr("SQLite Database *.db"), QDir::homePath());
    m_xbmcSqliteDatabaseDialog->setFileMode(QFileDialog::ExistingFile);

    connect(m_logFileDialog, SIGNAL(fileSelected(QString)), this, SLOT(onDebugLogPathChosen(QString)));
    connect(m_movieDirDialog, SIGNAL(fileSelected(QString)), this, SLOT(addMovieDir(QString)));
    connect(m_tvShowDirDialog, SIGNAL(fileSelected(QString)), this, SLOT(addTvShowDir(QString)));
    connect(m_xbmcThumbnailDirDialog, SIGNAL(fileSelected(QString)), this, SLOT(onChooseXbmcThumbnailPath(QString)));
    connect(m_xbmcSqliteDatabaseDialog, SIGNAL(fileSelected(QString)), this, SLOT(onChooseMediaCenterXbmcSqliteDatabase(QString)));

    connect(ui->buttonAddMovieDir, SIGNAL(clicked()), m_movieDirDialog, SLOT(open()));
    connect(ui->buttonRemoveMovieDir, SIGNAL(clicked()), this, SLOT(removeMovieDir()));
    connect(ui->movieDirs, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(movieListRowChanged(int)));
    connect(ui->buttonAddTvShowDir, SIGNAL(clicked()), m_tvShowDirDialog, SLOT(open()));
    connect(ui->buttonRemoveTvShowDir, SIGNAL(clicked()), this, SLOT(removeTvShowDir()));
    connect(ui->tvShowDirs, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(tvShowListRowChanged(int)));
    connect(ui->movieDirs, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(movieMediaCenterPathChanged(QTableWidgetItem*)));
    connect(ui->tvShowDirs, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(tvShowMediaCenterPathChanged(QTableWidgetItem*)));
    connect(ui->chkActivateDebug, SIGNAL(clicked()), this, SLOT(onActivateDebugMode()));
    connect(ui->buttonChooseLogfile, SIGNAL(clicked()), m_logFileDialog, SLOT(open()));
    connect(ui->logfilePath, SIGNAL(textChanged(QString)), this, SLOT(onSetDebugLogPath(QString)));

    connect(ui->radioXbmcXml, SIGNAL(clicked()), this, SLOT(onMediaCenterXbmcXmlSelected()));
    connect(ui->radioXbmcMysql, SIGNAL(clicked()), this, SLOT(onMediaCenterXbmcMysqlSelected()));
    connect(ui->radioXbmcSqlite, SIGNAL(clicked()), this, SLOT(onMediaCenterXbmcSqliteSelected()));
    connect(ui->buttonSelectSqliteDatabase, SIGNAL(clicked()), m_xbmcSqliteDatabaseDialog, SLOT(open()));
    connect(ui->buttonSelectThumbnailPath, SIGNAL(clicked()), m_xbmcThumbnailDirDialog, SLOT(open()));

    loadSettings();
}

/**
 * @brief Settings::~Settings
 */
Settings::~Settings()
{
    delete ui;
}

/**
 * @brief Returns an instance of the settings
 * @return Instance of Settings
 */
Settings *Settings::instance()
{
    return m_instance;
}

/**
 * @brief Loads all settings
 */
void Settings::loadSettings()
{
    // Globals
    m_mainWindowSize = m_settings.value("MainWindowSize").toSize();
    m_mainWindowPosition = m_settings.value("MainWindowPosition").toPoint();
    m_movieSplitterState = m_settings.value("MovieSplitterState").toByteArray();
    m_tvShowSplitterState = m_settings.value("TvShowSplitterState").toByteArray();
    m_movieSetsSplitterState = m_settings.value("MovieSetsSplitterState").toByteArray();
    m_debugModeActivated = m_settings.value("DebugModeActivated", false).toBool();
    m_debugLogPath = m_settings.value("DebugLogPath").toString();

    // Debug
    ui->chkActivateDebug->setChecked(m_debugModeActivated);
    ui->logfilePath->setText(m_debugLogPath);
    onActivateDebugMode();

    // Movie Directories
    m_movieDirectories.clear();
    ui->movieDirs->setRowCount(0);
    ui->movieDirs->clearContents();
    int moviesSize = m_settings.beginReadArray("Directories/Movies");
    for (int i=0 ; i<moviesSize ; ++i) {
        m_settings.setArrayIndex(i);
        ui->movieDirs->insertRow(i);
        QTableWidgetItem *item = new QTableWidgetItem(m_settings.value("path").toString());
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        item->setToolTip(m_settings.value("path").toString());
        QTableWidgetItem *item2 = new QTableWidgetItem;
        item2->setToolTip(tr("Movies are in separate folders"));
        if (m_settings.value("sepFolders", false).toBool())
            item2->setCheckState(Qt::Checked);
        else
            item2->setCheckState(Qt::Unchecked);
        ui->movieDirs->setItem(i, 0, item);
        ui->movieDirs->setItem(i, 1, new QTableWidgetItem(m_settings.value("mediaCenterPath").toString()));
        ui->movieDirs->setItem(i, 2, item2);
        SettingsDir dir;
        dir.path = m_settings.value("path").toString();
        dir.mediaCenterPath = m_settings.value("mediaCenterPath").toString();
        dir.separateFolders = m_settings.value("sepFolders", false).toBool();
        m_movieDirectories.append(dir);
    }
    m_settings.endArray();
    ui->buttonRemoveMovieDir->setEnabled(!m_movieDirectories.isEmpty());

    // TV Show Directories
    m_tvShowDirectories.clear();
    ui->tvShowDirs->setRowCount(0);
    ui->tvShowDirs->clearContents();
    int tvShowSize = m_settings.beginReadArray("Directories/TvShows");
    for (int i=0 ; i<tvShowSize ; ++i) {
        m_settings.setArrayIndex(i);
        ui->tvShowDirs->insertRow(i);
        QTableWidgetItem *item = new QTableWidgetItem(m_settings.value("path").toString());
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        item->setToolTip(m_settings.value("path").toString());
        ui->tvShowDirs->setItem(i, 0, item);
        ui->tvShowDirs->setItem(i, 1, new QTableWidgetItem(m_settings.value("mediaCenterPath").toString()));
        SettingsDir dir;
        dir.path = m_settings.value("path").toString();
        dir.mediaCenterPath = m_settings.value("mediaCenterPath").toString();
        m_tvShowDirectories.append(dir);
    }
    m_settings.endArray();
    ui->buttonRemoveTvShowDir->setEnabled(!m_tvShowDirectories.isEmpty());

    foreach (ScraperInterface *scraper, Manager::instance()->scrapers()) {
        if (scraper->hasSettings())
            scraper->loadSettings();
    }
    foreach (TvScraperInterface *scraper, Manager::instance()->tvScrapers()) {
        if (scraper->hasSettings())
            scraper->loadSettings();
    }

    // MediaCenterInterface
    m_mediaCenterInterface = m_settings.value("MediaCenterInterface", MediaCenterInterfaces::XbmcXml).toInt();
    if (m_mediaCenterInterface == MediaCenterInterfaces::XbmcXml)
        onMediaCenterXbmcXmlSelected();
    else if (m_mediaCenterInterface == MediaCenterInterfaces::XbmcMysql)
        onMediaCenterXbmcMysqlSelected();
    else if (m_mediaCenterInterface == MediaCenterInterfaces::XbmcSqlite)
        onMediaCenterXbmcSqliteSelected();

    m_xbmcMysqlHost      = m_settings.value("XbmcMysql/Host").toString();
    m_xbmcMysqlDatabase  = m_settings.value("XbmcMysql/Database").toString();
    m_xbmcMysqlUser      = m_settings.value("XbmcMysql/User").toString();
    m_xbmcMysqlPassword  = m_settings.value("XbmcMysql/Password").toString();
    m_xbmcSqliteDatabase = m_settings.value("XbmcSqlite/Database").toString();

    ui->inputDatabase->setText(m_xbmcMysqlDatabase);
    ui->inputHost->setText(m_xbmcMysqlHost);
    ui->inputUsername->setText(m_xbmcMysqlUser);
    ui->inputPassword->setText(m_xbmcMysqlPassword);
    ui->inputSqliteDatabase->setText(m_xbmcSqliteDatabase);
    m_xbmcSqliteDatabaseDialog->selectFile(m_xbmcSqliteDatabase);

    m_xbmcThumbnailPath = m_settings.value("XbmcThumbnailpath").toString();
    ui->inputThumbnailPath->setText(m_xbmcThumbnailPath);
    ui->inputThumbnailPath->setToolTip(m_xbmcThumbnailPath);
    m_xbmcThumbnailDirDialog->selectFile(m_xbmcThumbnailPath);
}

/**
 * @brief Saves all settings
 */
void Settings::saveSettings()
{
    bool mediaInterfaceChanged = false;

    m_settings.setValue("DebugModeActivated", m_debugModeActivated);
    m_settings.setValue("DebugLogPath", m_debugLogPath);

    m_settings.beginWriteArray("Directories/Movies");
    for (int i=0, n=m_movieDirectories.count() ; i<n ; ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("path", m_movieDirectories.at(i).path);
        m_settings.setValue("mediaCenterPath", m_movieDirectories.at(i).mediaCenterPath);
        m_settings.setValue("sepFolders", m_movieDirectories.at(i).separateFolders);
    }
    m_settings.endArray();

    m_settings.beginWriteArray("Directories/TvShows");
    for (int i=0, n=m_tvShowDirectories.count() ; i<n ; ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("path", m_tvShowDirectories.at(i).path);
        m_settings.setValue("mediaCenterPath", m_tvShowDirectories.at(i).mediaCenterPath);
    }
    m_settings.endArray();

    foreach (ScraperInterface *scraper, Manager::instance()->scrapers()) {
        if (scraper->hasSettings())
            scraper->saveSettings();
    }
    foreach (TvScraperInterface *scraper, Manager::instance()->tvScrapers()) {
        if (scraper->hasSettings())
            scraper->saveSettings();
    }

    if (ui->radioXbmcXml->isChecked()) {
        if (m_mediaCenterInterface != MediaCenterInterfaces::XbmcXml)
            mediaInterfaceChanged = true;
        m_mediaCenterInterface = MediaCenterInterfaces::XbmcXml;
    } else if (ui->radioXbmcMysql->isChecked()) {
        if (m_mediaCenterInterface != MediaCenterInterfaces::XbmcMysql)
            mediaInterfaceChanged = true;
        m_mediaCenterInterface = MediaCenterInterfaces::XbmcMysql;
    } else if (ui->radioXbmcSqlite->isChecked()) {
        if (m_mediaCenterInterface != MediaCenterInterfaces::XbmcSqlite)
            mediaInterfaceChanged = true;
        m_mediaCenterInterface = MediaCenterInterfaces::XbmcSqlite;
    }
    m_xbmcMysqlHost      = ui->inputHost->text();
    m_xbmcMysqlDatabase  = ui->inputDatabase->text();
    m_xbmcMysqlUser      = ui->inputUsername->text();
    m_xbmcMysqlPassword  = ui->inputPassword->text();
    m_xbmcSqliteDatabase = ui->inputSqliteDatabase->text();

    m_settings.setValue("XbmcMysql/Host", m_xbmcMysqlHost);
    m_settings.setValue("XbmcMysql/Database", m_xbmcMysqlDatabase);
    m_settings.setValue("XbmcMysql/User", m_xbmcMysqlUser);
    m_settings.setValue("XbmcMysql/Password", m_xbmcMysqlPassword);
    m_settings.setValue("XbmcSqlite/Database", m_xbmcSqliteDatabase);
    m_settings.setValue("MediaCenterInterface", m_mediaCenterInterface);
    m_settings.setValue("XbmcThumbnailpath", m_xbmcThumbnailPath);

    Manager::instance()->movieFileSearcher()->setMovieDirectories(this->movieDirectories());
    Manager::instance()->tvShowFileSearcher()->setMovieDirectories(this->tvShowDirectories());
    MessageBox::instance()->showMessage(tr("Settings saved"));

    Manager::instance()->setupMediaCenterInterface();
    if (mediaInterfaceChanged) {
        // TvShow File Searcher is started when Movie File Searcher has finished @see MainWindow.cpp
        Manager::instance()->movieFileSearcher()->start();
    }
}

/**
 * @brief Adds a movie directory
 * @param dir Directory to add
 */
void Settings::addMovieDir(QString dir)
{
    if (!dir.isEmpty()) {
        bool exists = false;
        for (int i=0, n=m_movieDirectories.count() ; i<n ; ++i) {
            if (m_movieDirectories.at(i).path == dir)
                exists = true;
        }

        if (!exists) {
            SettingsDir sDir;
            sDir.path = dir;
            m_movieDirectories.append(sDir);
            int row = ui->movieDirs->rowCount();
            ui->movieDirs->insertRow(row);
            QTableWidgetItem *item = new QTableWidgetItem(dir);
            item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            item->setToolTip(dir);
            QTableWidgetItem *itemCheck = new QTableWidgetItem();
            itemCheck->setCheckState(Qt::Unchecked);
            ui->movieDirs->setItem(row, 0, item);
            ui->movieDirs->setItem(row, 1, new QTableWidgetItem(""));
            ui->movieDirs->setItem(row, 2, itemCheck);
        }
    }
}

/**
 * @brief Removes a movie directory
 */
void Settings::removeMovieDir()
{
    int row = ui->movieDirs->currentRow();
    if (row < 0)
        return;

    m_movieDirectories.removeAt(row);
    ui->movieDirs->removeRow(row);
}

/**
 * @brief Enables/disables the button to remove a movie dir
 * @param currentRow Current row in the movie list
 */
void Settings::movieListRowChanged(int currentRow)
{
    ui->buttonRemoveMovieDir->setDisabled(currentRow < 0);
}

/**
 * @brief Adds a tv show dir
 * @param dir Directory to add
 */
void Settings::addTvShowDir(QString dir)
{
    if (!dir.isEmpty()) {
        bool exists = false;
        for (int i=0, n=m_tvShowDirectories.count() ; i<n ; ++i) {
            if (m_tvShowDirectories.at(i).path == dir)
                exists = true;
        }

        if (!exists) {
            SettingsDir sDir;
            sDir.path = dir;
            m_tvShowDirectories.append(sDir);
            int row = ui->tvShowDirs->rowCount();
            ui->tvShowDirs->insertRow(row);
            QTableWidgetItem *item = new QTableWidgetItem(dir);
            item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            item->setToolTip(dir);
            ui->tvShowDirs->setItem(row, 0, item);
            ui->tvShowDirs->setItem(row, 1, new QTableWidgetItem(""));
        }
    }
}

/**
 * @brief Removes a tv show dir
 */
void Settings::removeTvShowDir()
{
    int row = ui->tvShowDirs->currentRow();
    if (row<0)
        return;

    m_tvShowDirectories.removeAt(row);
    ui->tvShowDirs->removeRow(row);
}

/**
 * @brief Enables/Disables the button to remove a tv show dir
 * @param currentRow Current selected row in the list of tv show dirs
 */
void Settings::tvShowListRowChanged(int currentRow)
{
    ui->buttonRemoveTvShowDir->setDisabled(currentRow < 0);
}

/**
 * @brief Stores the values from the list for movie directories
 * @param item Current item
 */
void Settings::movieMediaCenterPathChanged(QTableWidgetItem *item)
{
    if (item->row() < 0 || item->row() >= m_movieDirectories.count())
        return;

    if (item->column() == 1)
        m_movieDirectories[item->row()].mediaCenterPath = item->text();
    else if (item->column() == 2)
        m_movieDirectories[item->row()].separateFolders = item->checkState() == Qt::Checked;
}

/**
 * @brief Stores the values from the list for tv show directories
 * @param item Current item
 */
void Settings::tvShowMediaCenterPathChanged(QTableWidgetItem *item)
{
    if (item->row() < 0 || item->row() >= m_tvShowDirectories.count() || item->column() != 1)
        return;
    m_tvShowDirectories[item->row()].mediaCenterPath = item->text();
}

/**
 * @brief Handles status of MediaCenter checkboxes and inputs
 */
void Settings::onMediaCenterXbmcXmlSelected()
{
    ui->radioXbmcXml->setChecked(true);
    ui->widgetXbmcMysql->setEnabled(false);
    ui->widgetXbmcSqlite->setEnabled(false);
    setXbmcThumbnailPathEnabled(false);
}

/**
 * @brief Handles status of MediaCenter checkboxes and inputs
 */
void Settings::onMediaCenterXbmcMysqlSelected()
{
    ui->radioXbmcMysql->setChecked(true);
    ui->widgetXbmcMysql->setEnabled(true);
    ui->widgetXbmcSqlite->setEnabled(false);
    setXbmcThumbnailPathEnabled(true);
}

/**
 * @brief Handles status of MediaCenter checkboxes and inputs
 */
void Settings::onMediaCenterXbmcSqliteSelected()
{
    ui->radioXbmcSqlite->setChecked(true);
    ui->widgetXbmcMysql->setEnabled(false);
    ui->widgetXbmcSqlite->setEnabled(true);
    setXbmcThumbnailPathEnabled(true);
}

/**
 * @brief Sets the SQLite database
 * @param file Database file
 */
void Settings::onChooseMediaCenterXbmcSqliteDatabase(QString file)
{
    if (!file.isEmpty()) {
        ui->inputSqliteDatabase->setText(file);
        m_xbmcSqliteDatabase = file;
    }
}

/**
 * @brief Shows a dialog to choose the thumbnail directory
 * @param dir Thumbnail directory to set
 */
void Settings::onChooseXbmcThumbnailPath(QString dir)
{
    if (!dir.isEmpty()) {
        m_xbmcThumbnailPath = dir;
        ui->inputThumbnailPath->setText(dir);
        ui->inputThumbnailPath->setToolTip(dir);
    }
}

/**
 * @brief Enables or disables the thumbnail path
 * @param enabled Status
 */
void Settings::setXbmcThumbnailPathEnabled(bool enabled)
{
    ui->inputThumbnailPath->setEnabled(enabled);
    ui->buttonSelectThumbnailPath->setEnabled(enabled);
    ui->labelXbmcThumbnailPath->setEnabled(enabled);
    ui->labelXbmcThumbnailPathDesc->setEnabled(enabled);
}

/**
 * @brief Toggles the status of logfile input and logfile select button based on the state of the checkbox
 */
void Settings::onActivateDebugMode()
{
    ui->logfilePath->setEnabled(ui->chkActivateDebug->isChecked());
    ui->buttonChooseLogfile->setEnabled(ui->chkActivateDebug->isChecked());
    m_debugModeActivated = ui->chkActivateDebug->isChecked();
}

/**
 * @brief Shows a file chooser to choose a path to a logfile
 */
void Settings::onDebugLogPathChosen(QString file)
{
    ui->logfilePath->setText(file);
}

/*** GETTER ***/

/**
 * @brief Returns the stored size of the main window
 * @return Size of the main window
 */
QSize Settings::mainWindowSize()
{
    return m_mainWindowSize;
}

/**
 * @brief Returns the stored position of the main window
 * @return Position of the main window
 */
QPoint Settings::mainWindowPosition()
{
    return m_mainWindowPosition;
}

/**
 * @brief Returns the state of the movie splitter
 * @return State of the movie splitter
 */
QByteArray Settings::movieSplitterState()
{
    return m_movieSplitterState;
}

/**
 * @brief Returns the state of the tv show splitter
 * @return State of the tv show splitter
 */
QByteArray Settings::tvShowSplitterState()
{
    return m_tvShowSplitterState;
}

/**
 * @brief Returns the state of the sets splitter
 * @return State of the sets splitter
 */
QByteArray Settings::movieSetsSplitterState()
{
    return m_movieSetsSplitterState;
}

/**
 * @brief Returns a list of movie directories
 * @return List of movie directories
 */
QList<SettingsDir> Settings::movieDirectories()
{
    return m_movieDirectories;
}

/**
 * @brief Returns a list of tv show directories
 * @return List of tv show directories
 */
QList<SettingsDir> Settings::tvShowDirectories()
{
    return m_tvShowDirectories;
}

/**
 * @brief Returns the number of the chosen MediaCenterInterface
 * @return Number of the chosen MediaCenterInterface
 */
int Settings::mediaCenterInterface()
{
    return m_mediaCenterInterface;
}

/**
 * @brief Returns the host of the MySQL database
 * @return Host of the MySQL db
 */
QString Settings::xbmcMysqlHost()
{
    return m_xbmcMysqlHost;
}

/**
 * @brief Returns the name of the MySQL database
 * @return Name of the MySQL db
 */
QString Settings::xbmcMysqlDatabase()
{
    return m_xbmcMysqlDatabase;
}

/**
 * @brief Returns the user of the MySQL database
 * @return User of the MySQL db
 */
QString Settings::xbmcMysqlUser()
{
    return m_xbmcMysqlUser;
}

/**
 * @brief Returns the password of the MySQL database
 * @return Password of the MySQL db
 */
QString Settings::xbmcMysqlPassword()
{
    return m_xbmcMysqlPassword;
}

/**
 * @brief Returns the path to the SQLite database
 * @return Path to SQLite database
 */
QString Settings::xbmcSqliteDatabase()
{
    return m_xbmcSqliteDatabase;
}

/**
 * @brief Returns the path to the thumbnails
 * @return Path to thumbnails
 */
QString Settings::xbmcThumbnailPath()
{
    return m_xbmcThumbnailPath;
}

/**
 * @brief Returns the state of the debug mode
 * @return Debug mode active or not
 */
bool Settings::debugModeActivated()
{
    return m_debugModeActivated;
}

/**
 * @brief Returns the path to the logfile
 * @return Path to logfile
 */
QString Settings::debugLogPath()
{
    return m_debugLogPath;
}

/*** SETTER ***/

/**
 * @brief Sets the size of the main window
 * @param mainWindowSize Size of the main window
 */
void Settings::setMainWindowSize(QSize mainWindowSize)
{
    m_mainWindowSize = mainWindowSize;
    m_settings.setValue("MainWindowSize", mainWindowSize);
}

/**
 * @brief Sets the position of the main window
 * @param mainWindowPosition Position of the main window
 */
void Settings::setMainWindowPosition(QPoint mainWindowPosition)
{
    m_mainWindowPosition = mainWindowPosition;
    m_settings.setValue("MainWindowPosition", mainWindowPosition);
}

/**
 * @brief Sets the state of the movie splitter
 * @param state State of the splitter
 */
void Settings::setMovieSplitterState(QByteArray state)
{
    m_movieSplitterState = state;
    m_settings.setValue("MovieSplitterState", state);
}

/**
 * @brief Sets the state of the tv show splitter
 * @param state State of the splitter
 */
void Settings::setTvShowSplitterState(QByteArray state)
{
    m_tvShowSplitterState = state;
    m_settings.setValue("TvShowSplitterState", state);
}

/**
 * @brief Sets the state of the movie sets splitter
 * @param state State of the splitter
 */
void Settings::setMovieSetsSplitterState(QByteArray state)
{
    m_movieSetsSplitterState = state;
    m_settings.setValue("MovieSetsSplitterState", state);
}

/**
 * @brief Sets the path to the logfile
 * @param path Path to logfile
 */
void Settings::onSetDebugLogPath(QString path)
{
    m_debugLogPath = path;
    m_logFileDialog->selectFile(m_debugLogPath);
}
