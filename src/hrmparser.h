#ifndef HRMPARSER_H
#define HRMPARSER_H

#include <QtCore/QXmlStreamReader>
#include <QtCore/qfile.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qregexp.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>

struct Time {
    char hh;
    char mm;
    char ss;
    char zz;

    static Time fromQTime(const QTime& qtime)
    {
        Time t;
        t.hh = qtime.hour();
        t.mm = qtime.minute();
        t.ss = qtime.second();
        t.zz = qtime.msec();
        return t;
    }
};

struct HRMEntry {
    //Time time;
    qint64 time;
    float hr;
    float speed;
    float altitude;
};

class HRMParser {
public:
    enum Section {
        Params,
        HRData,
        None
    };

    HRMParser(const QString &fileName) {
        m_fileName = fileName;
        m_lineNumber = -1;
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
        for (int i = 0; i < hrms.count(); ++i) {
            HRMEntry hrm = hrms.at(i);
            qint64 t = hrm.time;
            qDebug("hrm: %02d:%02d:%02d.%d %g, %g, %g", t.hh, t.mm, t.ss, t.zz, hrm.hr, hrm.speed/10, hrm.altitude);
        }
    }

    int interval() { return m_interval;}
    QTime startTime() { return m_time;}

    int hrAt(const QTime &time) {
        int msecsTo = m_time.msecsTo(time);
        int secsTo = msecsTo/1000;
        int index = secsTo/m_interval;
        if (index < 0 || index >= hrms.count()) {
            qDebug("hrAt: secsTo: %d, index: %d", secsTo, index);
            return 0;
        }
        HRMEntry entry = hrms.at(index);
        return int(entry.hr);
    }
private:
public:
    QString m_fileName;
    int m_lineNumber;
    int m_interval;
    QTime m_time;
    qint64 m_startTime;
    QVector<HRMEntry> hrms;
};

#endif // HRMPARSER_H
