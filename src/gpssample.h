#ifndef GPSSAMPLE_H
#define GPSSAMPLE_H
#include <QtCore/qvector.h>
#include <QtCore/qalgorithms.h>
#include <QtCore/qdebug.h>

struct GpsSample {
    GpsSample()
        : time(0), ele(0), lat(0), lon(0), hr(0), speed(-1)
    {
    }
                    // Supported by:
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
        return atTime(time) - constBegin();
    }
    
    qint64 startTime() const;
    qint64 endTime() const;
    
    float startAltitude() const;
    float endAltitude() const;

    float averageHR() const;
    int maximumHR() const;

    void correctTimeErrors();
    void print() const;
    void printSamples() const;
    bool writeGPX(const QString &fileName);

public:
    enum Activity {
        Unknown = 0,
        Cycling,
        Running
    };
    
    static const char *activityString(Activity activity)
    {
        static const char *activities[] = {"Unknown", "Cycling", "Running"};
        return activities[int(activity)];
    }
    struct MetaData {
        MetaData() : activity(SampleData::Unknown) {}
    
    public:
        SampleData::Activity activity;
        QString name;
        QString description;
    } metaData;

private:
    static bool lessThanTime(const GpsSample &a, const GpsSample &b) { return a.time <= b.time; }
};

QString msToDateTimeString(qint64 msSinceEpoch);
QString msToTimeString(qint64 msSinceMidnight);

#endif // GPSSAMPLE_H
