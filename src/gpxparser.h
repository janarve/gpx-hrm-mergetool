#ifndef GPXPARSER_H
#define GPXPARSER_H

#include <QtCore/qstring.h>
#include <QtCore/qvector.h>

class GpxParser
{
public:
    GpxParser(const QString &fileName);
    bool parse();
    float startAltitude() const;
    float endAltitude() const;
    qint64 startTime() const;
    qint64 endTime() const;

private:
    struct TrkPt {
        double lat;
        double lon;
        float ele;
        qint64 time;
    };
    QVector<TrkPt> m_trkpts;
    QString m_fileName;
};

#endif // GPXPARSER_H
