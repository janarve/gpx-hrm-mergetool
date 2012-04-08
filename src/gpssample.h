#ifndef GPSSAMPLE_H
#define GPSSAMPLE_H
#include <QtCore/qvector.h>
#include <QtCore/qalgorithms.h>
#include <QtCore/qdebug.h>

struct GpsSample {
    GpsSample()
        : time(0), ele(0), lat(0), lon(0), hr(0), speed(0)
    {
    }
    /*
    GpsSample(const GpsSample &other)
        : time(other.time), ele(other.ele), lat(other.lat), lon(other.lon),
          hr(other.hr), speed(other.speed)
    {
    }

    GpsSample &operator=(const GpsSample &other)
    {
        time = other.time;
        ele = other.ele;
        lat = other.lat;
        lon = other.lon;
        hr = other.hr;
        speed = other.speed;
        return *this;
    }
*/
    qint64 time;    // HRM+GPX
    float ele;      // HRM+GPX
    double lat;     // GPX
    double lon;     // GPX
    int hr;         // HRM
    float speed;    // HRM

};

QDebug operator<<(QDebug dbg, const GpsSample &s);

class SampleData : public QVector<GpsSample> {
public:
    QVector<GpsSample>::const_iterator atTime(qint64 time) const {
        GpsSample search;
        search.time = time;
        QVector<GpsSample>::const_iterator it = qLowerBound(constBegin(), constEnd(), search, &lessThanTime);
        if (it != constBegin())
            --it;
        return it;
    }

    int indexOfTime(qint64 time) const {
        QVector<GpsSample>::const_iterator it = atTime(time);
        return it - constBegin();
    }
    bool writeGPX(const QString &fileName);

    struct MetaData {
        bool isCycling;
        QString name;
    } metaData;


private:
    static bool lessThanTime(const GpsSample &a, const GpsSample &b)
    {
        return a.time <= b.time;
    }


};

QString msToDateTimeString(qint64 msSinceEpoch);
QString msToTimeString(qint64 msSinceMidnight);

#endif // GPSSAMPLE_H
