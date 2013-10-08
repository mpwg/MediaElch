#include "MovieSearchWidget.h"
#include "ui_MovieSearchWidget.h"

#include <QDebug>
#include "globals/Manager.h"
#include "scrapers/CustomMovieScraper.h"

MovieSearchWidget::MovieSearchWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MovieSearchWidget)
{
    ui->setupUi(this);
#if QT_VERSION >= 0x050000
    ui->results->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    ui->results->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
    ui->searchString->setType(MyLineEdit::TypeLoading);

    foreach (ScraperInterface *scraper, Manager::instance()->scrapers()) {
        ui->comboScraper->addItem(scraper->name(), scraper->identifier());
        connect(scraper, SIGNAL(searchDone(QList<ScraperSearchResult>)), this, SLOT(showResults(QList<ScraperSearchResult>)));
    }
    ui->comboScraper->setCurrentIndex(Settings::instance()->currentMovieScraper());

    connect(ui->comboScraper, SIGNAL(currentIndexChanged(int)), this, SLOT(search()));
    connect(ui->searchString, SIGNAL(returnPressed()), this, SLOT(search()));
    connect(ui->results, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(resultClicked(QTableWidgetItem*)));

    ui->chkActors->setMyData(MovieScraperInfos::Actors);
    ui->chkBackdrop->setMyData(MovieScraperInfos::Backdrop);
    ui->chkCertification->setMyData(MovieScraperInfos::Certification);
    ui->chkCountries->setMyData(MovieScraperInfos::Countries);
    ui->chkDirector->setMyData(MovieScraperInfos::Director);
    ui->chkGenres->setMyData(MovieScraperInfos::Genres);
    ui->chkOverview->setMyData(MovieScraperInfos::Overview);
    ui->chkPoster->setMyData(MovieScraperInfos::Poster);
    ui->chkRating->setMyData(MovieScraperInfos::Rating);
    ui->chkReleased->setMyData(MovieScraperInfos::Released);
    ui->chkRuntime->setMyData(MovieScraperInfos::Runtime);
    ui->chkSet->setMyData(MovieScraperInfos::Set);
    ui->chkStudios->setMyData(MovieScraperInfos::Studios);
    ui->chkTagline->setMyData(MovieScraperInfos::Tagline);
    ui->chkTitle->setMyData(MovieScraperInfos::Title);
    ui->chkTrailer->setMyData(MovieScraperInfos::Trailer);
    ui->chkWriter->setMyData(MovieScraperInfos::Writer);
    ui->chkLogo->setMyData(MovieScraperInfos::Logo);
    ui->chkClearArt->setMyData(MovieScraperInfos::ClearArt);
    ui->chkCdArt->setMyData(MovieScraperInfos::CdArt);
    ui->chkBanner->setMyData(MovieScraperInfos::Banner);
    ui->chkThumb->setMyData(MovieScraperInfos::Thumb);

    foreach (MyCheckBox *box, ui->groupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0)
            connect(box, SIGNAL(clicked()), this, SLOT(chkToggled()));
    }
    connect(ui->chkUnCheckAll, SIGNAL(clicked(bool)), this, SLOT(chkAllToggled(bool)));
}

MovieSearchWidget::~MovieSearchWidget()
{
    delete ui;
}

void MovieSearchWidget::clear()
{
    qDebug() << "Entered";
    ui->results->clearContents();
    ui->results->setRowCount(0);
}

void MovieSearchWidget::search(QString searchString)
{
    ui->comboScraper->setEnabled(true);
    ui->groupBox->setEnabled(true);
    m_currentCustomScraper = 0;
    m_customScraperIds.clear();
    ui->searchString->setText(searchString);
    search();
}

void MovieSearchWidget::search()
{
    qDebug() << "Entered";
    int index = ui->comboScraper->currentIndex();
    if (index < 0 || index >= Manager::instance()->scrapers().size()) {
        return;
    }
    m_scraperId = ui->comboScraper->itemData(index, Qt::UserRole).toString();
    ScraperInterface *scraper = Manager::instance()->scraper(m_scraperId);
    if (!scraper)
        return;

    if (m_scraperId == "custom-movie")
        m_currentCustomScraper = CustomMovieScraper::instance()->titleScraper();

    setChkBoxesEnabled(Manager::instance()->scraper(m_scraperId)->scraperSupports());
    clear();
    ui->comboScraper->setEnabled(false);
    ui->searchString->setLoading(true);
    scraper->search(ui->searchString->text());
    Settings::instance()->setCurrentMovieScraper(ui->comboScraper->currentIndex());
}

void MovieSearchWidget::showResults(QList<ScraperSearchResult> results)
{
    qDebug() << "Entered, size of results=" << results.count();
    ui->comboScraper->setEnabled(m_customScraperIds.isEmpty());
    ui->searchString->setLoading(false);
    ui->searchString->setFocus();
    foreach (const ScraperSearchResult &result, results) {
        QTableWidgetItem *item = new QTableWidgetItem(QString("%1 (%2)").arg(result.name).arg(result.released.toString("yyyy")));
        item->setData(Qt::UserRole, result.id);
        int row = ui->results->rowCount();
        ui->results->insertRow(row);
        ui->results->setItem(row, 0, item);
    }
}

void MovieSearchWidget::resultClicked(QTableWidgetItem *item)
{
    qDebug() << "Entered";

    if (m_scraperId == "custom-movie" || !m_customScraperIds.isEmpty()) {
        ui->comboScraper->setEnabled(false);
        ui->groupBox->setEnabled(false);

        if (m_currentCustomScraper == CustomMovieScraper::instance()->titleScraper())
            m_customScraperIds.clear();

        m_customScraperIds.insert(m_currentCustomScraper, item->data(Qt::UserRole).toString());
        QList<ScraperInterface*> scrapers = CustomMovieScraper::instance()->scrapersNeedSearch(infosToLoad(), m_customScraperIds);
        if (scrapers.isEmpty()) {
            m_scraperId = "custom-movie";
            emit sigResultClicked();
        } else {
            m_currentCustomScraper = scrapers.first();
            for (int i=0, n=ui->comboScraper->count() ; i<n ; ++i) {
                if (ui->comboScraper->itemData(i, Qt::UserRole).toString() == m_currentCustomScraper->identifier()) {
                    ui->comboScraper->setCurrentIndex(i);
                    break;
                }
            }
        }
    } else {
        m_scraperMovieId = item->data(Qt::UserRole).toString();
        m_customScraperIds.clear();
        emit sigResultClicked();
    }
}

void MovieSearchWidget::chkToggled()
{
    m_infosToLoad.clear();
    bool allToggled = true;
    foreach (MyCheckBox *box, ui->groupBox->findChildren<MyCheckBox*>()) {
        if (box->isChecked() && box->myData().toInt() > 0)
            m_infosToLoad.append(box->myData().toInt());
        if (!box->isChecked() && box->myData().toInt() > 0 && box->isEnabled())
            allToggled = false;
    }
    ui->chkUnCheckAll->setChecked(allToggled);

    QString scraperId = ui->comboScraper->itemData(ui->comboScraper->currentIndex(), Qt::UserRole).toString();
    Settings::instance()->setScraperInfos(WidgetMovies, scraperId, m_infosToLoad);
}

void MovieSearchWidget::chkAllToggled(bool toggled)
{
    foreach (MyCheckBox *box, ui->groupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0 && box->isEnabled())
            box->setChecked(toggled);
    }
    chkToggled();
}

QString MovieSearchWidget::scraperId()
{
    return m_scraperId;
}

QString MovieSearchWidget::scraperMovieId()
{
    return m_scraperMovieId;
}

QList<int> MovieSearchWidget::infosToLoad()
{
    return m_infosToLoad;
}

void MovieSearchWidget::setChkBoxesEnabled(QList<int> scraperSupports)
{
    QString scraperId = ui->comboScraper->itemData(ui->comboScraper->currentIndex(), Qt::UserRole).toString();
    QList<int> infos = Settings::instance()->scraperInfos(WidgetMovies, scraperId);

    foreach (MyCheckBox *box, ui->groupBox->findChildren<MyCheckBox*>()) {
        box->setEnabled(scraperSupports.contains(box->myData().toInt()));
        box->setChecked((infos.contains(box->myData().toInt()) || infos.isEmpty()) && scraperSupports.contains(box->myData().toInt()));
    }
    chkToggled();
}

QMap<ScraperInterface*, QString> MovieSearchWidget::customScraperIds()
{
    return m_customScraperIds;
}