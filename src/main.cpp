#include <QtCore>
#include <QtGui>

#include <stdio.h>

#include <hrmcom.h>

#include "gpxparser.h"
#include "hrmparser.h"
#include "geolocationiterator.h"
#include "geolocationinterpolator.h"

void usage()
{
    printf("usage:\n"
           "  hrmgpx [options] <hrmFile> [gpxFile]\n"
           "\n"
           "Options:\n"
           " --fetch-hrm        Download HR data from Polar watch\n"
           " --error-correction Try to detect errors and correct them\n"
           " --ignore-gpx-timestamps Use HRM speeds to create trackpoints in a route\n"
           );
}

int readHRMData(HWND hWnd)
{
    int error = 0;
    printf("Connecting to Polar monitor...\n");
    if (!fnHRMCom_ResetIRCommunication (0))
    {
        // Resetting IR connection was not successful
        printf("Could not reset HRMCOM");
        error = -1;
    }

    if (error >= 0) {
        if (!fnHRMCom_StartIRCommunication (HRMCOM_PARAM_IRDA, L"IR"))
        {
            // IrDA couldn't be opened, stop connection thread
            printf("Could not start IR comm");
            error =  -2;
        }
    }

    POLAR_SSET_MONITORINFO	psmi;
    if (error >= 0) {
        POLAR_SSET_GENERAL		psg;

        // Fill general information
        ZeroMemory (&psg, sizeof(psg));

        psg.iSize          = sizeof (psg);
        psg.iConnection    = HRMCOM_CONNECTION_IR;
        psg.bConnectionDlg = TRUE;
        psg.hOwnerWnd      = hWnd;	// Owner window handle

        // Reset monitor information
        ::ZeroMemory (&psmi, sizeof(psmi));

        // Read monitor info from HR monitor using IR
        if (!fnHRMCom_ReadMonitorInfo (&psg, &psmi))
        {
            printf("Could not read monitor data");
            error = -3;
        } else {
            printf("Monitor:  %d\n"
                   "Submodel: %d\n"
                   ,psmi.iMonitorInUse, psmi.iMonitorSubModel);
        }
    }

    printf("Please wait while downloading HRM data from monitor...\n");
    if (error >= 0) {
        // Read exercise files from HR monitor
        if (!fnHRMCom_ReadExercisesData (hWnd, FALSE))
        {
            printf("could not read exercises");
            error = -4;
        }
    }

    int newImportedFilesCount = 0;
    int invalidImportedFilesCount = 0;
    if (error >= 0) {
        QDir cwd = QDir::current();
        if (!cwd.exists(QLatin1String("hrm")))
            cwd.mkdir(QLatin1String("hrm"));
        const QString outputPath = cwd.absoluteFilePath(QLatin1String("hrm"));

        POLAR_EXERCISEFILE	pef;
        for (int exerciseIndex = 0; exerciseIndex < psmi.iTotalFiles; ++exerciseIndex) {
            if (!fnHRMCom_GetExeFileInfo (exerciseIndex, &pef)) {
                error = -5;
                break;
            }
            if (!fnHRMCom_AnalyzeFile (exerciseIndex, 0)) {
                error = -6;
                break;
            }

            QTime t(0,0);
            const QString strTime = t.addMSecs(pef.iTime * 1000).toString(QLatin1String("hhmmss"));

            const QString fileName = QString::fromLatin1("%1/%2-%3.hrm").arg(outputPath).arg(pef.iDate).arg(strTime);
            const QByteArray name = fileName.toLatin1();
            if (pef.iDuration == 0) {
                printf("Error in file %s\n", name.constData());
                ++invalidImportedFilesCount;
            } else {
                if (!QFile::exists(fileName)) {
                    printf("Saving file %s\n", qPrintable(fileName));
                    if (!fnHRMCom_SaveExerciseHRM (hWnd, (LPTSTR)name.constData(), 0)) {
                        error = -7;
                        break;
                    }
                    ++newImportedFilesCount;
                }
            }
        }
    }
    if (error < 0) {
        printf("ERROR: (errorcode: %d)\n", error);
    } else {
        printf("Imported %d new files of a total of %d exercise files\n", newImportedFilesCount, psmi.iTotalFiles);
        if (invalidImportedFilesCount)
            printf("Warning: %d invalid files ignored\n", invalidImportedFilesCount);
    }

    // close conn
    fnHRMCom_EndIRCommunication (FALSE);

    return error;
}

int mergeTracks(const QString &hrmFile, const QString &gpxFilename, bool errorCorrection = false, bool ignoreGpxTimestamps = false)
{
    printf("ignoreGpxTimestamps:%d, \n", ignoreGpxTimestamps);
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

    QFile gpxFile(QStringLiteral("output.gpx"));
    if (!gpxFile.open(QIODevice::WriteOnly)) {
        return -1;
    }
    if (!saveGPX(mergedSamples, &gpxFile))
        return -1;
    gpxFile.close();
    return 0;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    bool fetch_hrm = false;
    bool error_correction = false;
    bool ignore_gpx_timestamps = false;
    QString gpxFilename, hrmFile;
    bool firstPass = true;
    foreach (const QString &arg, app.arguments()) {
        if (firstPass) {
            firstPass = false;
            continue;
        }
        if (arg == QLatin1String("--fetch-hrm")) {
            fetch_hrm = true;
        } else if (arg == QLatin1String("--error-correction")) {
            error_correction = true;
        } else if (arg == QLatin1String("--ignore-gpx-timestamps")) {
            ignore_gpx_timestamps = true;
        } else {
            if (hrmFile.isNull())
                hrmFile = arg;
            else
                gpxFilename = arg;
        }
    }
    
    if (fetch_hrm) {
        readHRMData(0);
    } else if (!hrmFile.isNull()) {
/*
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(replyFinished(QNetworkReply*)));
        manager->get(QNetworkRequest(QUrl("http://qt.nokia.com")));
*/
        mergeTracks(hrmFile, gpxFilename, error_correction, ignore_gpx_timestamps);
    } else {
        usage();
    }


    //merge(args.at(1), args.at(2), args.at(3));
    return app.exit();
}
