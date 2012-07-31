#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qfile.h>
#include "gpssample.h"

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
