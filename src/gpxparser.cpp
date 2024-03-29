#include "gpxparser.h"
#include <QtCore/QXmlStreamReader>
#include <QtCore/qfile.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qregularexpression.h>


class GpxStreamReader : public QXmlStreamReader
{
public:
    GpxStreamReader(QIODevice *device) : QXmlStreamReader(device) {}
    bool read(SampleData *sampleData);
private:
    bool isWhiteSpace() const
    {
       return isCharacters() && text().toString().trimmed().isEmpty();
    }
};

bool GpxStreamReader::read(SampleData *sampleData)
{
    bool ok = true;
    bool timeElement = false;
    bool eleElement = false;

    GpsSample trkpt;
    while (!atEnd()) {
        QXmlStreamReader::TokenType tt = readNext();
        if (error()) {
            qWarning("error at line: %lld (%d %s)\n", lineNumber(), (int)error(), qPrintable(errorString()));
            return false;
        } else {
            if (tt == QXmlStreamReader::StartElement && name() == "metadata") {
                while (!atEnd()) {
                    tt = readNext();
                    if (isEndElement()) {
                        break;
                    } else if (isWhiteSpace()) {
                        // ignore these, just whitespace
                    } else {
                        if (tt == QXmlStreamReader::StartElement && name() == "name") {
                            sampleData->metaData.name = readElementText();
                        } else if (tt == QXmlStreamReader::StartElement && name() == "desc") {
                            sampleData->metaData.description = readElementText();
                        } else if (tt == QXmlStreamReader::StartElement) {
                            skipCurrentElement();
                        }
                    }
                }
            }
            if (tt == QXmlStreamReader::StartElement && name() == "time") {
                timeElement = true;
            } else if (tt == QXmlStreamReader::StartElement && name() == "ele") {
                eleElement = true;
            } else if (tt == QXmlStreamReader::StartElement && name() == "trkpt") {
                QXmlStreamAttributes attr = attributes();
                double lat, lon;
                lat = attr.value("lat").toString().toDouble(&ok);
                if (ok)
                    lon = attr.value("lon").toString().toDouble(&ok);
                if (!ok) {
                    qWarning("(%lld): Error reading longitude and latitude data", lineNumber());
                    break;
                }
                trkpt.lat = lat;
                trkpt.lon = lon;
            } else if (tt == QXmlStreamReader::EndElement && name() == "trkpt") {
                sampleData->append(trkpt);
            } else if (tt == QXmlStreamReader::Characters) {
                if (eleElement) {
                    float ele = text().toString().toFloat(&ok);
                    if (!ok) {
                        qWarning("(%lld): Error reading elevation data", lineNumber());
                        break;
                    }
                    trkpt.ele = ele;
                    eleElement = false;
                } else if (timeElement) {
                    QString timeStr = text().toString();
                    //yyyy-MM-ddThh:mm:ss(.z)
                    // Local? time: yyyy-MM-ddThh:mm:ss(.z)
                    // UTC  time  : yyyy-MM-ddThh:mm:ss(.z)Z
                    // Timezone   : yyyy-MM-ddThh:mm:ss(.z)+03:00
                    //yyyy-MM-ddThh:mm:ss(.z)?(Z|+hh:mm)
                    //2013-07-09T15:26:48Z
                    uint milliseconds = 0;
                    const int indexOfDot = timeStr.indexOf(QLatin1Char('.'));

                    if (indexOfDot >= 0) {
                        int i = indexOfDot;
                        ++i;
                        while (i < timeStr.count() && timeStr.at(i++).isDigit()) {  }

                        // The spec is unclear whether there can be more than 3 fractional digits
                        const QString strMS = timeStr.mid(indexOfDot + 1, qMax(3, i - indexOfDot - 1));
                        if (!strMS.isEmpty()) {
                            static int powers[] = {100, 10, 1};
                            const int numberAfterDot = strMS.toUInt(&ok);
                            if (!ok)
                                break;
                            const int digits = strMS.count();
                            Q_ASSERT(digits >= 1);
                            milliseconds = numberAfterDot * powers[digits - 1];
                            timeStr.remove(indexOfDot, i - indexOfDot);
                        }
                    }
                    QDateTime dt = QDateTime::fromString(timeStr,Qt::ISODate);
                    ok = dt.isValid();
                    if (!ok) {
                        qWarning("(%d): Error reading time data", lineNumber());
                        break;
                    }
                    trkpt.time = dt.toMSecsSinceEpoch();
                    trkpt.time += milliseconds;
                    timeElement = false;
                }
            }
        }
    }

    return ok;
}

bool loadGPX(SampleData *sampleData, QIODevice *device)
{
    GpxStreamReader reader(device);
    return reader.read(sampleData);
}

bool saveGPX(const SampleData &sampleData, QIODevice *device)
{
    if (sampleData.isEmpty()) {
        qWarning("Data contains no samples");
        return false;
    }
    QTextStream stream(device);
    stream.setCodec("UTF-8");

    const GpsSample &sample = sampleData.first();
    QDateTime dt;
    dt.setMSecsSinceEpoch(sample.time);
    const QString strDateTime = dt.toString(QLatin1String("dd/MM/yyyy hh:mm:ss"));

    stream.setRealNumberNotation(QTextStream::FixedNotation);
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << "<gpx xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1"
             " http://www.topografix.com/GPX/1/1/gpx.xsd"
             " http://www.garmin.com/xmlschemas/GpxExtensions/v3"
             " http://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd"
             " http://www.garmin.com/xmlschemas/TrackPointExtension/v1"
             " http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd\""
           " version=\"1.1\""
           " creator=\"HrmGpx\""
           " xmlns=\"http://www.topografix.com/GPX/1/1\""
           " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
           " xmlns:gpxtpx=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\""
           " xmlns:gpxx=\"http://www.garmin.com/xmlschemas/GpxExtensions/v3\">\n";
    stream << "  <metadata>\n";
    // In case the name contains the time, replace it with the new time
    // This is how www.sports-tracker.com does it.
    // 27/05/2013/15:24:01.000
    QRegularExpression re(QStringLiteral("\\d\\d/\\d\\d/\\d{4}/\\d\\d:\\d\\d:\\d\\d\\.\\d+$"));
    const GpsSample first = sampleData.first();
    QString name = sampleData.metaData.name;
    QString strTime = msToDateTimeStringHuman(first.time);
    name.replace(re, strTime);
    stream << "    <name>" << name << "</name>\n";
    stream << "    <desc>" << sampleData.metaData.description << "</desc>\n";
    stream << "    <author>\n";
    stream << "      <name>Jan Arve S\xe6ther</name>\n";    //&aelig;
    stream << "    </author>\n";
    stream << "    <link href=\"sjarve@gmail.com\">\n";
    stream << "      <text>HrmGpx</text>\n";
    stream << "    </link>\n";
    stream << "  </metadata>\n";
    stream << "  <trk>\n";
    stream << "      <trkseg>\n";

    Q_FOREACH (const GpsSample &sample, sampleData) {
        stream.setRealNumberPrecision(15);
        stream << "        <trkpt lat=\"" << sample.lat << "\" lon=\"" << sample.lon << "\">\n";
        stream.setRealNumberPrecision(1);
        stream << "          <ele>" << sample.ele << "</ele>\n";
        stream << "          <time>" << msToDateTimeString(sample.time) << "</time>\n";
        stream << "          <!--speed>" << sample.speed << "</speed-->\n";
        stream << "          <extensions><gpxtpx:TrackPointExtension><gpxtpx:hr>"
                                << sample.hr
                                << "</gpxtpx:hr></gpxtpx:TrackPointExtension></extensions>\n";
        stream << "        </trkpt>\n";
    }

    stream << "    </trkseg>\n";
    stream << "  </trk>\n";
    stream << "</gpx>\n";
    return true;
}
