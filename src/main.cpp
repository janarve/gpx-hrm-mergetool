#include <QtCore>
#include <QtGui>

#include <stdio.h>

#include "gpxparser.h"
#include "hrmparser.h"
#include "geolocationiterator.h"
#include "geolocationinterpolator.h"

#include <float.h>

#ifdef HAVE_HRMCOM
# include "polardevice.h"
#endif
void usage()
{
    printf("usage:\n"
           "  hrmgpx [options] <hrmFile> [gpxFile]\n"
           "\n"
           "Options:\n"
#ifdef HAVE_HRMCOM
           " --fetch-hrm                    Download HR data from Polar watch\n"
#endif
           " --error-correction             Try to detect errors and correct them\n"
           " --ignore-gpx-timestamps        Use HRM speeds to create trackpoints in a route\n"
           " --altitude <start>[:<end>]     Adjust altitude to <start>, <end> or both\n"
           );
}


int mergeTracks(const QString &hrmFile, const QString &gpxFilename, bool errorCorrection = false, bool ignoreGpxTimestamps = false,
                float startAltitude = -FLT_MAX, float endAltitude = -FLT_MAX)
{
    SampleData gpxSampleData;
    if (!gpxFilename.isNull()) {
        QFile gpxFile(gpxFilename);
        if (gpxFile.open(QIODevice::ReadOnly)) {
            printf("Analyzing GPX file: %s\n", qPrintable(gpxFilename));
            if (!loadGPX(&gpxSampleData, &gpxFile))
                return -1;
            gpxSampleData.print();
        }
        gpxFile.close();
    }
    printf("Analyzing HRM file: %s\n", qPrintable(hrmFile));
    SampleData hrmSampleData;

    HRMReader hrmReader(hrmFile);
    if (hrmReader.read(&hrmSampleData)) {
        hrmSampleData.print();
        printf("Interval:       %d\n", hrmReader.interval());
        printf("Samples:        %d\n", hrmSampleData.count());
    }

    if (startAltitude != -FLT_MAX || endAltitude != -FLT_MAX)
        hrmSampleData.correctAltitudes(startAltitude, endAltitude);

    SampleData mergedSamples;

    if (gpxSampleData.isEmpty()) {
        mergedSamples = hrmSampleData;
    } else {
        mergedSamples.metaData.activity = hrmSampleData.metaData.activity;
        mergedSamples.metaData.name = gpxSampleData.metaData.name;
        mergedSamples.metaData.description = gpxSampleData.metaData.description;
        qint64 hrmStartTime = hrmReader.startTime();
        qint64 hrmEndTime = hrmReader.endTime();
        if (ignoreGpxTimestamps) {
            GeoLocationIterator gpxIter(&gpxSampleData);
            GeoLocationInterpolator interpolator(gpxIter);
            for (int i = 0; i < hrmSampleData.count(); ++i) {
                GpsSample &hrmSample = hrmSampleData[i];
                float speed = hrmSample.speed;  // km/h
                qint64 time = hrmSample.time;
                qint64 speedDuration = 0;
                if (i == 0 && i < hrmSampleData.count() - 1) {
                    speedDuration = hrmSampleData.at(i + 1).time - time;
                } else {
                    speedDuration = time - hrmSampleData.at(i - 1).time;
                }
                double hrmDist = speed * speedDuration/3600.0;    // meters
                double lat, lon;
                interpolator.advance(hrmDist, &lat, &lon);
                hrmSample.lat = lat;
                hrmSample.lon = lon;
                mergedSamples << hrmSample;
            }
        } else {

            int gpxStart = gpxSampleData.indexOfTime(hrmStartTime);
            int gpxEnd = gpxSampleData.indexOfTime(hrmEndTime);

            for (int i = gpxStart; i < gpxEnd; ++i) {
                GpsSample sample = gpxSampleData.at(i);
                if (i == gpxStart)
                    sample.time = hrmStartTime;
                if (i == gpxEnd - 1) {
                    if (sample.time < hrmEndTime)
                        sample.time = hrmEndTime;
                }
                if (sample.time > hrmEndTime) {
                    sample.time = hrmEndTime;
                    i = gpxEnd;     // finish iteration and leave loop
                }
                const GpsSample hrmSample = *hrmSampleData.atTime(sample.time);

                const float hr = hrmSample.hr;
                const float speed = hrmSample.speed;

                sample.hr = hr;
                sample.speed = speed;
                mergedSamples << sample;
            }
        }
    }
    
    if (errorCorrection)
        mergedSamples.correctTimeErrors();
    printf("Result of merge:\n");
    if (mergedSamples.count())
        mergedSamples.print();


    /* gOOGLE Elevation API
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
         this, SLOT(replyFinished(QNetworkReply*)));

    manager->get(QNetworkRequest(QUrl("http://qt.nokia.com")));
    */
   /*
    int lastGoodEle = -1;
    for (int i = 0; i < mergedSamples.count(); ++i) {
        GpsSample &sample = mergedSamples[i];
        const GpsSample hrmSample = *hrmParser.samples.atTime(sample.time);
        float gpxEle = sample.ele;
        float hrmEle = hrmSample.ele;
        if (qAbs(gpxEle - hrmEle) > 10) {
            lastGoodEle = i - 1;
        } else {
            if (lastGoodEle < i - 1) {
                const GpsSample lastGoodGpxSample = mergedSamples.samples.at(lastGoodEle);
                const GpsSample lastGoodHrmSample = *hrmParser.samples.atTime(lastGoodGpxSample.time);

                const float hrmBaseAlt = lastGoodHrmSample.ele;
                const float gpxBaseAlt = lastGoodGpxSample.ele;
                const float deltaHrmEle = hrmEle - hrmBaseAlt;
                const float deltaGpxEle = gpxEle - gpxBaseAlt;
                const float factor = deltaGpxEle/deltaHrmEle;   // 13/11
                // gpxAlt = hrmBaseAlt + factor * (hrmAlt - hrmBaseAlt)

                for (int j = lastGoodEle; j < i; ++j) {
                    float gpxAlt = hrmBaseAlt + factor * (hrmAlt - hrmBaseAlt);
                    sample.ele = gpxAlt;
                }
            }
            lastGoodEle = i;
        }

    }
    */
    /*
       hrm     10   13   17   18   21   /11
       gpx      9    x    x    x   22   /13

       hrm           3    7    8   11   /11
       gpx      9   12,5 17,3 18,5 22

       x =

      3/11 = x/13
                  */
    QFileInfo fi(gpxFilename);
    qint64 startTime = mergedSamples.startTime();
    QDateTime dt;
    dt.setMSecsSinceEpoch(startTime);
    const QString dateString = dt.toString(QLatin1String("yyyyMMdd"));
    const QString outputFileName(QString::fromLatin1("combined/%1-%2.gpx").arg(dateString, fi.baseName()));
    QFile gpxFile(outputFileName);
    if (!gpxFile.open(QIODevice::WriteOnly)) {
        return -1;
    }
    if (!saveGPX(mergedSamples, &gpxFile))
        return -1;
    gpxFile.close();
    printf("Merged file written to: %s\n", qPrintable(outputFileName));
    return 0;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
#ifdef HAVE_HRMCOM
    bool fetch_hrm = false;
#endif
    bool error_correction = false;
    bool ignore_gpx_timestamps = false;
    QString gpxFilename, hrmFile;
    bool firstPass = true;
    bool altitudeDataIsHere = false;
    bool commandLineOk = true;
    float startAltitude = -FLT_MAX;
    float endAltitude = -FLT_MAX;
    foreach (const QString &arg, app.arguments()) {
        if (firstPass) {
            firstPass = false;
            continue;
        }
        if (arg == QLatin1String("--altitude")) {
            altitudeDataIsHere = true;
#ifdef HAVE_HRMCOM
        } else if (arg == QLatin1String("--fetch-hrm")) {
            fetch_hrm = true;
#endif
        } else if (arg == QLatin1String("--error-correction")) {
            error_correction = true;
        } else if (arg == QLatin1String("--ignore-gpx-timestamps")) {
            ignore_gpx_timestamps = true;
        } else {
            if (altitudeDataIsHere) {
                // Hilton: 160.44
                // Nydalen: 100.26
                QStringList altitudes = arg.split(QLatin1Char(':'));
                if (altitudes.count() >= 1) {
                    if (!altitudes.at(0).isEmpty())
                        startAltitude = altitudes.at(0).toFloat(&commandLineOk);
                }
                if (commandLineOk && altitudes.count() == 2) {
                    if (!altitudes.at(1).isEmpty())
                        endAltitude = altitudes.at(1).toFloat(&commandLineOk);
                }

                if (startAltitude == -FLT_MAX && endAltitude == -FLT_MAX) {
                    commandLineOk = false;
                    break;
                }
                altitudeDataIsHere = false;
            } else {
                if (hrmFile.isNull())
                    hrmFile = arg;
                else
                    gpxFilename = arg;
            }
        }
    }
    
    if (commandLineOk) {
        if (0) {
#ifdef HAVE_HRMCOM
        } else if (fetch_hrm) {
            readHRMData(0);
#endif
        } else if (!hrmFile.isNull()) {
            mergeTracks(hrmFile, gpxFilename, error_correction, ignore_gpx_timestamps, startAltitude, endAltitude);
        } else {
            usage();
        }
    } else {
        usage();
    }

    app.exit();
    return 0;
}
