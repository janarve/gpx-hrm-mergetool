#include "gpxparser.h"
#include <QtCore/QXmlStreamReader>
#include <QtCore/qfile.h>
#include <QtCore/qdatetime.h>

GpxParser::GpxParser(const QString &fileName)
{
    m_fileName = fileName;
}

float GpxParser::startAltitude() const
{
    if (m_trkpts.count() > 0)
        return m_trkpts.first().ele;
    return -1.0;
}

float GpxParser::endAltitude() const
{
    if (m_trkpts.count() > 0)
        return m_trkpts.last().ele;
    return -1.0;
}

qint64 GpxParser::startTime() const
{
    if (m_trkpts.count() > 0)
        return m_trkpts.first().time;
    return -1.0;
}

qint64 GpxParser::endTime() const
{
    if (m_trkpts.count() > 0)
        return m_trkpts.last().time;
    return -1.0;
}

bool GpxParser::parse()
{
    QFile file(m_fileName);
    bool ok = false;
    if (file.open(QIODevice::ReadOnly)) {
        ok = true;
        QXmlStreamReader reader(&file);
        bool timeElement = false;
        bool eleElement = false;

        while (!reader.atEnd()) {
            QXmlStreamReader::TokenType tt = reader.readNext();
            if (reader.error()) {
                qWarning("error at line: %d (%s)", reader.lineNumber(), qPrintable(reader.errorString()));
                return false;
            } else {
                TrkPt trkpt;
                if (tt == QXmlStreamReader::StartElement && reader.name() == "time") {
                    timeElement = true;
                } else if (tt == QXmlStreamReader::StartElement && reader.name() == "ele") {
                    eleElement = true;
                } else if (tt == QXmlStreamReader::StartElement && reader.name() == "trkpt") {
                    QXmlStreamAttributes attr = reader.attributes();
                    double lat, lon;
                    lat = attr.value("lat").toString().toDouble(&ok);
                    if (ok)
                        lon = attr.value("lon").toString().toDouble(&ok);
                    if (!ok) {
                        qWarning("%s(%d): Error reading longitude and latitude data", qPrintable(m_fileName), reader.lineNumber());
                        break;
                    }
                    trkpt.lat = lat;
                    trkpt.lon = lon;
                } else if (tt == QXmlStreamReader::EndElement && reader.name() == "trkpt") {
                    m_trkpts << trkpt;
                } else if (tt == QXmlStreamReader::Characters) {
                    if (eleElement) {
                        float ele = reader.text().toString().toFloat(&ok);
                        if (!ok) {
                            qWarning("%s(%d): Error reading elevation data", qPrintable(m_fileName), reader.lineNumber());
                            break;
                        }
                        trkpt.ele = ele;
                        eleElement = false;
                    } else if (timeElement) {
                        QString timeStr = reader.text().toString();

                        QString format("yyyy-MM-ddThh:mm:ss.z");    // note z is centiseconds!!
                        QDateTime dt = QDateTime::fromString(timeStr, format);
                        if (!dt.isValid()) {
                            QString format("yyyy-MM-ddThh:mm:ss");    // somtimes centiseconds can be omitted...
                            dt = QDateTime::fromString(timeStr, format);
                        }
                        ok = dt.isValid();
                        if (!ok) {
                            qWarning("%s(%d): Error reading time data", qPrintable(m_fileName), reader.lineNumber());
                            break;
                        }
                        trkpt.time = dt.toMSecsSinceEpoch();
                        timeElement = false;
                    }
                }
            }
        }
    }

    return ok;
}
