#include "MovieXmlReader.h"

#include "movies/Movie.h"

#include <QDate>
#include <QDomDocument>
#include <QDomNodeList>
#include <QStringList>
#include <QUrl>

namespace mediaelch {
namespace kodi {

MovieXmlReader::MovieXmlReader(Movie& movie) : m_movie{movie}
{
}

void MovieXmlReader::parseNfoDom(QDomDocument domDoc)
{
    if (domDoc.elementsByTagName("movie").isEmpty()) {
        qWarning() << "[MovieXmlReader] No <movie> tag in the document";
        return;
    }
    QDomElement movieElement = domDoc.elementsByTagName("movie").at(0).toElement();
    QMap<QString, void (MovieXmlReader::*)(QDomElement)> tagParsers;
    // clang-format off
    tagParsers.insert("title",         &MovieXmlReader::simpleString<&Movie::setName>);
    tagParsers.insert("originaltitle", &MovieXmlReader::simpleString<&Movie::setOriginalName>);
    tagParsers.insert("sorttitle",     &MovieXmlReader::simpleString<&Movie::setSortTitle>);
    tagParsers.insert("plot",          &MovieXmlReader::simpleString<&Movie::setOverview>);
    tagParsers.insert("outline",       &MovieXmlReader::simpleString<&Movie::setOutline>);
    tagParsers.insert("tagline",       &MovieXmlReader::simpleString<&Movie::setTagline>);
    tagParsers.insert("set",           &MovieXmlReader::movieSet);
    tagParsers.insert("actor",         &MovieXmlReader::movieActor);
    tagParsers.insert("thumb",         &MovieXmlReader::movieThumbnail);
    tagParsers.insert("fanart",        &MovieXmlReader::movieFanart);
    tagParsers.insert("playcount",     &MovieXmlReader::simpleInt<&Movie::setPlayCount>);
    tagParsers.insert("top250",        &MovieXmlReader::simpleInt<&Movie::setTop250>);
    tagParsers.insert("tag",           &MovieXmlReader::simpleString<&Movie::addTag>);
    tagParsers.insert("studio",        &MovieXmlReader::stringList<&Movie::addStudio, '/'>);
    tagParsers.insert("genre",         &MovieXmlReader::stringList<&Movie::addGenre, '/'>);
    tagParsers.insert("country",       &MovieXmlReader::stringList<&Movie::addCountry, '/'>);
    tagParsers.insert("ratings",       &MovieXmlReader::movieRatingV17);
    tagParsers.insert("rating",        &MovieXmlReader::movieRatingV16);
    tagParsers.insert("votes",         &MovieXmlReader::movieVoteCountV16);
    // clang-format on

    QDomNodeList nodes = movieElement.childNodes();
    for (int i = 0; i < nodes.size(); ++i) {
        if (nodes.at(i).isElement()) {
            QDomElement element = nodes.at(i).toElement();
            if (tagParsers.contains(element.tagName())) {
                // call the stored method pointer
                (this->*tagParsers[element.tagName()])(element);
            }
        }
    }

    if (!domDoc.elementsByTagName("year").isEmpty()) {
        m_movie.setReleased(QDate::fromString(domDoc.elementsByTagName("year").at(0).toElement().text(), "yyyy"));
    }
    // will overwrite the release date set by <year>
    if (!domDoc.elementsByTagName("premiered").isEmpty()) {
        QString value = domDoc.elementsByTagName("premiered").at(0).toElement().text();
        m_movie.setReleased(QDate::fromString(value, "yyyy-MM-dd"));
    }

    if (!domDoc.elementsByTagName("runtime").isEmpty()) {
        m_movie.setRuntime(std::chrono::minutes(domDoc.elementsByTagName("runtime").at(0).toElement().text().toInt()));
    }
    if (!domDoc.elementsByTagName("mpaa").isEmpty()) {
        m_movie.setCertification(Certification(domDoc.elementsByTagName("mpaa").at(0).toElement().text()));
    }
    if (!domDoc.elementsByTagName("lastplayed").isEmpty()) {
        QDateTime lastPlayed = QDateTime::fromString(
            domDoc.elementsByTagName("lastplayed").at(0).toElement().text(), "yyyy-MM-dd HH:mm:ss");
        if (!lastPlayed.isValid()) {
            lastPlayed =
                QDateTime::fromString(domDoc.elementsByTagName("lastplayed").at(0).toElement().text(), "yyyy-MM-dd");
        }
        m_movie.setLastPlayed(lastPlayed);
    }

    if (!domDoc.elementsByTagName("dateadded").isEmpty()) {
        QDateTime dateadded = QDateTime::fromString(
            domDoc.elementsByTagName("dateadded").at(0).toElement().text(), "yyyy-MM-dd HH:mm:ss");
        if (dateadded.isValid()) {
            m_movie.setDateAdded(dateadded);
        }
    }

    // v17/v18 tmdbid
    if (!domDoc.elementsByTagName("id").isEmpty()) {
        m_movie.setId(ImdbId(domDoc.elementsByTagName("id").at(0).toElement().text()));
    }
    // v16 tmdbid
    if (!domDoc.elementsByTagName("tmdbid").isEmpty()) {
        m_movie.setTmdbId(TmdbId(domDoc.elementsByTagName("tmdbid").at(0).toElement().text()));
    }
    // v17 ids
    auto uniqueIds = domDoc.elementsByTagName("uniqueid");
    for (int i = 0; i < uniqueIds.size(); ++i) {
        QDomElement element = uniqueIds.at(i).toElement();
        QString type = element.attribute("type");
        QString value = element.text().trimmed();
        if (type == "imdb") {
            m_movie.setId(ImdbId(value));
        } else if (type == "tmdb") {
            m_movie.setTmdbId(TmdbId(value));
        }
    }

    if (!domDoc.elementsByTagName("trailer").isEmpty()) {
        m_movie.setTrailer(QUrl(domDoc.elementsByTagName("trailer").at(0).toElement().text()));
    }
    if (!domDoc.elementsByTagName("watched").isEmpty()) {
        m_movie.setWatched(domDoc.elementsByTagName("watched").at(0).toElement().text() == "true" ? true : false);
    } else {
        m_movie.setWatched(m_movie.playcount() > 0);
    }

    QStringList writers;
    for (int i = 0, n = domDoc.elementsByTagName("credits").size(); i < n; i++) {
        for (const QString& writer :
            domDoc.elementsByTagName("credits").at(i).toElement().text().split(",", QString::SkipEmptyParts)) {
            writers.append(writer.trimmed());
        }
    }
    m_movie.setWriter(writers.join(", "));

    QStringList directors;
    for (int i = 0, n = domDoc.elementsByTagName("director").size(); i < n; i++) {
        for (const QString& director :
            domDoc.elementsByTagName("director").at(i).toElement().text().split(",", QString::SkipEmptyParts)) {
            directors.append(director.trimmed());
        }
    }
    m_movie.setDirector(directors.join(", "));
}

void MovieXmlReader::movieSet(QDomElement movieSetElement)
{
    const QDomNodeList setNameElements = movieSetElement.elementsByTagName("name");
    const QDomNodeList setOverviewElements = movieSetElement.elementsByTagName("overview");

    // We need to support both the old and new XML syntax.
    //
    // New Kodi v17 XML Syntax:
    // <set>
    //   <name>Movie Set Name</name>
    //   <overview></overview>
    // </set>
    //
    // Old Syntax:
    // <set>Movie Set Name</set>
    //
    MovieSet set;
    if (!setNameElements.isEmpty()) {
        set.name = setNameElements.at(0).toElement().text();
    } else {
        set.name = movieSetElement.text();
    }
    if (!setOverviewElements.isEmpty()) {
        set.overview = setOverviewElements.at(0).toElement().text();
    }
    m_movie.setSet(set);
}

void MovieXmlReader::movieActor(QDomElement actorElement)
{
    Actor a;
    a.imageHasChanged = false;
    if (!actorElement.elementsByTagName("name").isEmpty()) {
        a.name = actorElement.elementsByTagName("name").at(0).toElement().text();
    }
    if (!actorElement.elementsByTagName("role").isEmpty()) {
        a.role = actorElement.elementsByTagName("role").at(0).toElement().text();
    }
    if (!actorElement.elementsByTagName("thumb").isEmpty()) {
        a.thumb = actorElement.elementsByTagName("thumb").at(0).toElement().text();
    }
    m_movie.addActor(a);
}

void MovieXmlReader::movieThumbnail(QDomElement thumbElement)
{
    Poster p;
    p.originalUrl = QUrl(thumbElement.toElement().text());
    p.thumbUrl = QUrl(thumbElement.toElement().attribute("preview"));
    m_movie.images().addPoster(p);
}

void MovieXmlReader::movieFanart(QDomElement fanartElement)
{
    QDomNodeList thumbs = fanartElement.elementsByTagName("thumb");
    for (int i = 0; i < thumbs.size(); ++i) {
        QDomElement thumbElement = thumbs.at(i).toElement();
        Poster p;
        p.originalUrl = QUrl(thumbElement.text());
        p.thumbUrl = QUrl(thumbElement.attribute("preview"));
        m_movie.images().addBackdrop(p);
    }
}

void MovieXmlReader::movieRatingV17(QDomElement element)
{
    // <ratings>
    //   <rating name="default" default="true">
    //     <value>10</value>
    //     <votes>10</votes>
    //   </rating>
    // </ratings>
    auto ratings = element.elementsByTagName("rating");
    for (int i = 0; i < ratings.length(); ++i) {
        Rating rating;
        auto ratingElement = ratings.at(i).toElement();
        rating.source = ratingElement.attribute("name", "default");
        bool ok = false;
        int max = ratingElement.attribute("max", "0").toInt(&ok);
        if (ok && max > 0) {
            rating.maxRating = max;
        }
        rating.rating = ratingElement.elementsByTagName("value").at(0).toElement().text().replace(",", ".").toDouble();
        rating.voteCount =
            ratingElement.elementsByTagName("votes").at(0).toElement().text().replace(",", "").replace(".", "").toInt();
        m_movie.ratings().push_back(rating);
        m_movie.setChanged(true);
    }
}

void MovieXmlReader::movieRatingV16(QDomElement element)
{
    // <rating>10.0</rating>
    QString value = element.text();
    if (!value.isEmpty()) {
        Rating rating = m_movie.ratings().empty() ? Rating{} : m_movie.ratings().first();
        rating.rating = value.replace(",", ".").toDouble();
        m_movie.ratings().first() = rating;
        m_movie.setChanged(true);
    }
}

void MovieXmlReader::movieVoteCountV16(QDomElement element)
{
    // <votes>100</votes>
    QString value = element.text();
    if (!value.isEmpty()) {
        Rating rating = m_movie.ratings().empty() ? Rating{} : m_movie.ratings().first();
        rating.voteCount = value.replace(",", ".").replace(".", "").toInt();
        m_movie.ratings().first() = rating;
        m_movie.setChanged(true);
    }
}

} // namespace kodi
} // namespace mediaelch