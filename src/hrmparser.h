#ifndef HRMPARSER_H
#define HRMPARSER_H

#include <QtCore/QXmlStreamReader>
#include <QtCore/qfile.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qregexp.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>

#include "gpssample.h"


class HRMParser {
public:
    enum Section {
        Params,
        HRData,
        None
    };

    HRMParser(const QString &fileName)
        : m_lineNumber(-1), m_startTime(-1), m_length(-1), m_interval(-1), m_isCyclingData(0)
    {
        m_fileName = fileName;
    }
    bool parse();

    float startAltitude() const;
    float endAltitude() const;
    qint64 startTime() const;
    qint64 endTime() const;

    void error(const char *str)
    {
        printf("%s(%d): Error (%s)", qPrintable(m_fileName), m_lineNumber, str);
    }

    void dumpAsGPX() const
    {
        QFile file("output.gpx");
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning("Could not open file for writing");
            return;
        }

        QXmlStreamWriter w(&file);
        w.writeStartDocument();
        QString xsi(QLatin1String("http://www.w3.org/2001/XMLSchema-instance"));
        QString gpxtpx(QLatin1String("http://www.garmin.com/xmlschemas/TrackPointExtension/v1"));
        //gpxx="http://www.garmin.com/xmlschemas/GpxExtensions/v3"
        QString xmlns(QLatin1String("http://www.topografix.com/GPX/1/1"));
        w.writeDefaultNamespace(xmlns);
        w.writeStartElement("gpx");
            w.writeStartElement("metadata");
            w.writeEndElement();
            w.writeStartElement("trk");
                w.writeStartElement("trkseg");
        w.writeEndElement();
        file.close();
    }

    void dump() const
    {
        qDebug("interval: %d", m_interval);
        for (int i = 0; i < samples.count(); ++i) {
            GpsSample sample = samples.at(i);
            QString strTime = msToDateTimeString(sample.time);
            qDebug("hrm: %s %g, %g, %g", qPrintable(strTime), sample.hr, sample.speed/10, sample.ele);
        }
    }

    int interval() const { return m_interval;}
    bool isCycling() const { return samples.metaData.isCycling; }

private:
public:
    int m_lineNumber;
    qint64 m_startTime;
    qint64 m_length;
    int m_interval;
    bool m_isCyclingData;
    QTime m_time;
    SampleData samples;
    QString m_fileName;
};

#endif // HRMPARSER_H
