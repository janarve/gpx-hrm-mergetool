#include <QtCore>

#include "gpxparser.h"
#include "hrmparser.h"

#include <stdio.h>



void merge(const QString &hrmFileName, const QString &gpxFileName, const QString &mergedFileName)
{
    HRMParser parser(hrmFileName);
    parser.parse();
    //parser.dump();
    QFile file(gpxFileName);
    file.open(QIODevice::ReadOnly);
    QFile of(mergedFileName);
    of.open(QIODevice::WriteOnly);

    QXmlStreamReader reader(&file);

    QXmlStreamWriter writer(&of);

    QTime prevTime;
    QString prevTimeStr;
    bool timeElement = false;
    bool eleElement = false;
    bool firstElement = true;
    QString gpxtpx(QLatin1String("http://www.garmin.com/xmlschemas/TrackPointExtension/v1"));
    QString xmlns(QLatin1String("http://www.topografix.com/GPX/1/1"));
    int prevHR = -1;
    QString lastLat, lastLon, lastEle;
    while (!reader.atEnd()) {
        QXmlStreamReader::TokenType tt = reader.readNext();
        if (reader.error()) {
            qWarning("error at line: %d (%s)", reader.lineNumber(), qPrintable(reader.errorString()));
            return;
        } else {
            if (firstElement && tt == QXmlStreamReader::StartElement) {
                writer.writeNamespace(gpxtpx, "gpxtpx");
                firstElement = false;
            } else if (eleElement && tt == QXmlStreamReader::Characters) {
                lastEle = reader.text().toString();
                eleElement = false;
            } else if (timeElement && tt == QXmlStreamReader::Characters) {
                QString timeStr = reader.text().toString();
                prevTimeStr = timeStr;
                QString format("yyyy-MM-ddThh:mm:ss.z");    // note z is centiseconds!!
                QDateTime dt = QDateTime::fromString(timeStr, format);
                if (!dt.isValid()) {
                    QString format("yyyy-MM-ddThh:mm:ss");    // somtimes centiseconds can be omitted...
                    dt = QDateTime::fromString(timeStr, format);
                }
                QTime time = dt.time();
                int centiSecs = time.msec();
                time.addMSecs(centiSecs*10 - centiSecs);
                prevTime = time;
                timeElement = false;
            } else if (tt == QXmlStreamReader::EndElement && reader.name() == "trkpt") {
                int hr = parser.hrAt(prevTime);
                if (hr == 0) {
                    qDebug("suspicious line: %d", reader.lineNumber());
                    if (prevHR >= 0)
                        hr = prevHR;
                }
                if (hr != prevHR) {
                    writer.writeStartElement(xmlns, "extensions");
                        writer.writeStartElement(gpxtpx, "TrackPointExtension");
                        writer.writeTextElement(gpxtpx, "hr", QString::fromAscii("%1").arg(hr));
                        writer.writeEndElement();
                    writer.writeEndElement();
                }
                if (hr != 0)
                    prevHR = hr;
            } else if (tt == QXmlStreamReader::EndElement && reader.name() == "trkseg") {
                writer.writeStartElement(xmlns, "trkpt");
                writer.writeAttribute(xmlns, "lat", lastLat);
                writer.writeAttribute(xmlns, "lon", lastLon);
                writer.writeTextElement(xmlns, "ele", lastEle);
                writer.writeTextElement(xmlns, "time", prevTimeStr);
                writer.writeEndElement();
            /*
                ### tricky...... need to backtrack the stream!

                writer.writeStartElement(xmlns, "extensions");
                    writer.writeStartElement(gpxtpx, "TrackPointExtension");
                    writer.writeTextElement(gpxtpx, "hr", QString::fromAscii("%1").arg(prevHR));
                    writer.writeEndElement();
                writer.writeEndElement();
                */
            } else if (tt == QXmlStreamReader::StartElement && reader.name() == "time") {
                timeElement = true;
            } else if (tt == QXmlStreamReader::StartElement && reader.name() == "ele") {
                eleElement = true;
            } else if (tt == QXmlStreamReader::StartElement && reader.name() == "trkpt") {
                QXmlStreamAttributes attr = reader.attributes();
                lastLat = attr.value("lat").toString();
                lastLon = attr.value("lon").toString();
            }
            writer.writeCurrentToken(reader);
        }
    }

    of.close();
    file.close();
}

void usage()
{
    qWarning("usage:\n"
             "  gpxhrm <gpxFile> <hrmFile>");
}

static QString msToDateTimeString(qint64 msSinceEpoch)
{
    QDateTime dt;
    dt.setMSecsSinceEpoch(msSinceEpoch);
    QString str = dt.toString(QLatin1String("yyyy-MM-dd hh:mm:ss.zzz"));
    return str;
}

static QString msToTimeString(qint64 ms)
{
    QTime t(0,0);
    t = t.addMSecs((int)ms);
    QString str = t.toString(QLatin1String("hh:mm:ss.zzz"));
    return str;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    if (args.count() < 3) {
        usage();
        return 0;
    }
    qWarning("Input HRM file: %s\n"
             "Input GPX file: %s\n", qPrintable(args.at(1)), qPrintable(args.at(2)));

    GpxParser gpxParser(args.at(1));
    if (gpxParser.parse()) {
        qint64 startTime = gpxParser.startTime();
        qint64 endTime = gpxParser.endTime();
        qint64 elapsedTime = endTime - startTime;

        QString startStr = msToDateTimeString(startTime);
        QString endStr = msToDateTimeString(endTime);
        QString elapsedStr = msToTimeString(elapsedTime);

        printf("Start time:     %s\n", qPrintable(startStr));
        printf("End time:       %s\n", qPrintable(endStr));
        printf("Elapsed time:   %s (%d)\n", qPrintable(elapsedStr), (int)elapsedTime);
        printf("Start altitude: %g\n", gpxParser.startAltitude());
        printf("End altitude:   %g\n", gpxParser.endAltitude());
    }
    HRMParser hrmParser(args.at(2));
    if (hrmParser.parse()) {

    }
    //merge(args.at(1), args.at(2), args.at(3));
    return app.exit();

}
