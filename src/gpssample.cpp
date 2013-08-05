#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qfile.h>
#include "gpssample.h"
#include "geo.h"

QString msToDateTimeStringHuman(qint64 msSinceEpoch)
{
    QDateTime dt;
    dt.setMSecsSinceEpoch(msSinceEpoch);
    QString str = dt.toString(QLatin1String("dd/MM/yyyy/hh:mm:ss.zzz"));    //Local timezone
    return str;
}

QString msToDateTimeString(qint64 msSinceEpoch)
{
    QDateTime dt;
    dt.setMSecsSinceEpoch(msSinceEpoch);
    QString str = dt.toString(QLatin1String("yyyy-MM-ddThh:mm:ss.zzz"));    //Local timezone
    return str;
}

QString msToTimeString(qint64 msSinceMidnight)
{
    QTime t(0,0);
    t = t.addMSecs((int)msSinceMidnight);
    QString str = t.toString(QLatin1String("hh:mm:ss.zzz"));
    return str;
}

QDebug operator<<(QDebug dbg, const GpsSample &s)
{
    dbg << "time:" << msToDateTimeString(s.time);
    dbg << "(lat,lon):(" << s.lat << s.lon << ")";
    dbg << "elevation:" << s.ele;
    dbg << "hr:" << s.hr;
    dbg << "speed:" << s.speed;
    return dbg;
}

float SampleData::startAltitude() const
{
    if (!isEmpty())
        return first().ele;
    return -1.0;
}

float SampleData::endAltitude() const
{
    if (!isEmpty())
        return last().ele;
    return -1.0;
}

qint64 SampleData::startTime() const
{
    if (!isEmpty())
        return first().time;
    return -1.0;
}

qint64 SampleData::endTime() const
{
    if (!isEmpty())
        return last().time;
    return -1.0;
}

float SampleData::averageHR() const {
    float sum = 0;
    foreach (const GpsSample &sample, *this) {
        sum += sample.hr;
    }
    return sum/count();
}

int SampleData::maximumHR() const {
    int max = 0;
    foreach (const GpsSample &sample, *this)
        max = qMax(sample.hr, max);
    return max;
}

void SampleData::correctTimeErrors()
{
    double prevSpeed = 0;
    double prevSpeedDelta = 0;
    if (!isEmpty()) {
        GpsSample prev = first();
        int lastGoodIndex = -1;
        for (int i = 1; i < count(); ++i) {
            GpsSample &curr = operator[](i);
            double dist = haversineDistance(prev.lat, prev.lon, curr.lat, curr.lon);
            double speed = -1;
            if (curr.time != prev.time)
                speed = dist * 3600000.0 /(curr.time - prev.time);

            curr.speed = speed;
            if (i >= 1) {
                if (lastGoodIndex != -1) {
                    GpsSample lastGood = at(lastGoodIndex);
                    double speedDelta = qAbs(speed - lastGood.speed);
                    //printf("i: %d, lastGoodIndex: %d, speedDelta: %.2f, %.2f, %.2f, %.2f\n", i, lastGoodIndex, speedDelta, speed, lastGood.speed, dist);
                    if (speedDelta < 20) {
                        if (lastGoodIndex < i - 1) {
                            // Backtrack with a smaller error threshold (10 km/h difference)
                            const int lastBackTrackIndex = qMax(lastGoodIndex - 10, 0);
                            while (lastGoodIndex > lastBackTrackIndex) {
                                GpsSample &s1 = operator[](lastGoodIndex - 1);
                                GpsSample &s2 = operator[](lastGoodIndex);
                                if (qAbs(s1.speed - s2.speed) > 8) {
                                    --lastGoodIndex;
                                } else {
                                    break;
                                }
                            }
                            lastGood = at(lastGoodIndex);
                            qint64 deltaTime = curr.time - lastGood.time;
                            double deltaLat = curr.lat - lastGood.lat;
                            double deltaLon = curr.lon - lastGood.lon;
                            int deltaIndex = i - lastGoodIndex;
                            printf("Invalid data in range [%d,%d], fixing with interpolation\n", lastGoodIndex + 1, i-1);
                            // Do linear interpolation over the error range
                            for (int j = lastGoodIndex + 1; j < i; ++j) {
                                GpsSample &fix = operator[](j);
                                fix.time = lastGood.time + deltaTime * (j - lastGoodIndex)/deltaIndex;
                                fix.lat = lastGood.lat + deltaLat * (j - lastGoodIndex)/deltaIndex;
                                fix.lon = lastGood.lon + deltaLon * (j - lastGoodIndex)/deltaIndex;
                            }
                        }
                        lastGoodIndex = i;
                    }
                } else {
                    lastGoodIndex = i;
                }
                prevSpeed = speed;
            }
            prev = curr;
        }
    }
}

/*!
    Adjusts the altitudes so that the altitude profile starts at \a startAltitude
    and ends at \a endAltitude, while keeping the general profile.
    This is useful for e.g. calibrating altitude data read with a baryometer.

    If \a startAltitude or \a endAltitude is -FLT_MAX,
    all altitudes will be moved with the same height so that either the starting point
    or the ending point matches.
*/
void SampleData::correctAltitudes(float startAltitude, float endAltitude = -FLT_MAX)
{
    if (!isEmpty() && (startAltitude != -FLT_MAX || endAltitude != -FLT_MAX)) {

        const float sampledStartEle = first().ele;
        const float sampledEndEle = last().ele;

        float startDelta = (startAltitude == -FLT_MAX ? 0 : startAltitude - sampledStartEle); // 160 - 213 = -53
        float endDelta = (endAltitude == -FLT_MAX ? startDelta : endAltitude - sampledEndEle);       // 140 - 153 = -13
        if (startAltitude == -FLT_MAX)
            startDelta = endDelta;

        const float ascent = (endDelta - startDelta)/count();
        for (int i = 0; i < count(); ++i) {
            GpsSample &curr = operator[](i);
            curr.ele += startDelta - ascent * i;
        }
    }
}


void SampleData::print() const
{
    qint64 timeStarted = startTime();
    qint64 timeFinsihed = endTime();
    qint64 timeElapsed = timeFinsihed - timeStarted;

    QString startStr = msToDateTimeString(timeStarted);
    QString endStr = msToDateTimeString(timeFinsihed);
    QString elapsedStr = msToTimeString(timeElapsed);

    printf("Start time:     %s\n", qPrintable(startStr));
    printf("End time:       %s\n", qPrintable(endStr));
    printf("Elapsed time:   %s\n", qPrintable(elapsedStr));
    printf("Start altitude: %g\n", startAltitude());
    printf("End altitude:   %g\n", endAltitude());
    printf("Activity:       %s\n", activityString(metaData.activity));
    printf("HR avg/max:     %.1f/%d\n", averageHR(), maximumHR());

    double maxSpeed = 0;
    int maxSpeedIndex = -1;
    double totalDist = 0;
    if (!isEmpty()) {
        GpsSample prev = first();
        if (prev.speed == -1) {
            double speed = 0.0;
            for (int i = 1; i < count(); ++i) {
                GpsSample curr = at(i);
                double dist = haversineDistance(prev.lat, prev.lon, curr.lat, curr.lon);
                totalDist += dist;
                speed = dist * 3600000.0 /(curr.time - prev.time);
                prev.speed = speed;
                if (speed > maxSpeed) {
                    maxSpeedIndex = i;
                    maxSpeed = speed;
                }
                prev = curr;
            }
            prev.speed = speed;
        }
        for (int i = 0; i < count(); ++i) {
            GpsSample curr = at(i);
            double speed = curr.speed;
            if (speed > maxSpeed) {
                maxSpeedIndex = i;
                maxSpeed = speed;
            }
        }
    }
    printf("Max speed:      %.1f\n", maxSpeed);
    printf("Max speed index:%d\n", maxSpeedIndex);
    if (totalDist > 0)
        printf("Route distance :%.2f\n", totalDist);
}

void SampleData::printSamples() const
{
    for (int i = 0; i < count(); ++i) {
        GpsSample sample = at(i);
        QString strTime = msToDateTimeString(sample.time);
        printf("hrm: %s %g, %g, %g", qPrintable(strTime), sample.hr, sample.speed/10, sample.ele);
    }
}


bool SampleData::writeGPX(const QString &fileName)
{
    QFile out(fileName);
    if (!out.open(QIODevice::WriteOnly)) {
        qWarning("Failed to open '%s'", qPrintable(fileName));
        return false;
    }

    if (count() == 0) {
        qWarning("Data contains no samples");
        return false;
    }
    QTextStream stream(&out);

    const GpsSample &sample = first();
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
    stream << "    <name>" << metaData.name << "</name>\n";
    stream << "    <desc>" << metaData.description << "</desc>\n";
    stream << "    <author>\n";
    stream << "      <name>Jan-Arve Saether</name>\n";
    stream << "    </author>\n";
    stream << "    <link href=\"jans@tihlde.org\">\n";
    stream << "      <text>HrmGpx</text>\n";
    stream << "    </link>\n";
    stream << "  </metadata>\n";
    stream << "  <trk>\n";
    stream << "      <trkseg>\n";

    Q_FOREACH (const GpsSample &sample, (*this)) {
        stream.setRealNumberPrecision(15);
        stream << "        <trkpt lat=\"" << sample.lat << "\" lon=\"" << sample.lon << "\">\n";
        stream.setRealNumberPrecision(1);
        stream << "          <ele>" << sample.ele << "</ele>\n";
        stream << "          <time>" << msToDateTimeString(sample.time) << "</time>\n";
        stream << "          <extensions><gpxtpx:TrackPointExtension><gpxtpx:hr>"
                                << sample.hr
                                << "</gpxtpx:hr></gpxtpx:TrackPointExtension></extensions>\n";
        stream << "        </trkpt>\n";
    }

    stream << "    </trkseg>\n";
    stream << "  </trk>\n";
    stream << "</gpx>\n";
    out.close();
    return true;
}
