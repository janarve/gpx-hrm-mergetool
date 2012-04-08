#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <stdio.h>

#include <hrmcom.h>

#include "gpxparser.h"
#include "hrmparser.h"
#include "dialog.h"

void usage()
{
    printf("usage:\n"
           "  gpxhrm <gpxFile> <hrmFile>\n"
           "\n"
           "Options:\n"
           " --fetch-hrm        Download HR data from Polar watch\n"
           );
}

int readHRMData(HWND hWnd)
{
    int error = 0;
    printf("Starting readHRMData...");
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

    if (error >= 0) {
        // Read exercise files from HR monitor
        if (!fnHRMCom_ReadExercisesData (hWnd, FALSE))
        {
            printf("could not read exercises");
            error = -4;
        }
    }

    if (error >= 0) {
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

            QDir cwd = QDir::current();
            if (!cwd.exists(QLatin1String("files")))
                cwd.mkdir(QLatin1String("files"));
            QString fileName = cwd.absoluteFilePath(QLatin1String("files"));
            fileName.append(QString::fromAscii("/%1-%2-%3.hrm").arg(pef.iExerciseID).arg(pef.iDate).arg(pef.iTime));
            QByteArray name = fileName.toAscii();
            printf("Saving file %s\n", qPrintable(fileName));
            if (!fnHRMCom_SaveExerciseHRM (hWnd, (LPTSTR)name.constData(), 0)) {
                error = -7;
                break;
            }
        }
    }
    if (error >= 0) {
        printf("Imported %d exercise files\n", psmi.iTotalFiles);
    }

    // close conn
    fnHRMCom_EndIRCommunication (FALSE);

    return error;
}

int mergeTracks(const QString &gpxFile, const QString &hrmFile)
{
    printf("Analyzing GPX file: %s\n", qPrintable(gpxFile));

    GpxParser gpxParser(gpxFile);
    if (gpxParser.parse()) {
        qint64 startTime = gpxParser.startTime();
        qint64 endTime = gpxParser.endTime();
        qint64 elapsedTime = endTime - startTime;

        QString startStr = msToDateTimeString(startTime);
        QString endStr = msToDateTimeString(endTime);
        QString elapsedStr = msToTimeString(elapsedTime);

        printf("Start time:     %s\n", qPrintable(startStr));
        printf("End time:       %s\n", qPrintable(endStr));
        printf("Elapsed time:   %s\n", qPrintable(elapsedStr));
        printf("Start altitude: %g\n", gpxParser.startAltitude());
        printf("End altitude:   %g\n", gpxParser.endAltitude());
    }

    printf("Analyzing HRM file: %s\n", qPrintable(hrmFile));
    HRMParser hrmParser(hrmFile);
    if (hrmParser.parse()) {
        qint64 startTime = hrmParser.startTime();
        qint64 endTime = hrmParser.endTime();
        qint64 elapsedTime = endTime - startTime;

        QString startStr = msToDateTimeString(startTime);
        QString endStr = msToDateTimeString(endTime);
        QString elapsedStr = msToTimeString(elapsedTime);

        printf("Start time:     %s\n", qPrintable(startStr));
        printf("End time:       %s\n", qPrintable(endStr));
        printf("Elapsed time:   %s\n", qPrintable(elapsedStr));
        printf("Start altitude: %g\n", hrmParser.startAltitude());
        printf("End altitude:   %g\n", hrmParser.endAltitude());
        printf("Interval:       %d\n", hrmParser.m_interval);
        printf("Samples:        %d\n", hrmParser.samples.count());
    }

    qint64 hrmStartTime = hrmParser.startTime();
    qint64 hrmEndTime = hrmParser.endTime();

    int gpxStart = gpxParser.samples.indexOfTime(hrmStartTime);
    int gpxEnd = gpxParser.samples.indexOfTime(hrmEndTime);

    SampleData mergedSamples;

    for (int i = gpxStart; i < gpxEnd; ++i) {
        GpsSample sample = gpxParser.samples.at(i);

        //int index = hrmParser.samples.indexOfTime(sample.time);
        //const GpsSample hrmSample = hrmParser.samples.at(index);
        const GpsSample hrmSample = *hrmParser.samples.atTime(sample.time);

        const float hr = hrmSample.hr;
        const float speed = hrmSample.speed;

        sample.hr = hr;
        sample.speed = speed;
        mergedSamples << sample;
    }
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
    mergedSamples.writeGPX(QLatin1String("output.gpx"));
    return 0;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    bool fetch_hrm = false;
    printf("test\n");
    QString gpxFile, hrmFile;
    bool firstPass = true;
    foreach (const QString &arg, app.arguments()) {
        if (firstPass) {
            firstPass = false;
            continue;
        }
        if (arg == QLatin1String("--fetch-hrm")) {
            fetch_hrm = true;
        } else {
            if (gpxFile.isNull())
                gpxFile = arg;
            else
                hrmFile = arg;
        }
    }
    printf("test\n");
    if (fetch_hrm) {
        readHRMData(0);
    } else if (!gpxFile.isNull() && !hrmFile.isNull()) {
        mergeTracks(gpxFile, hrmFile);
    } else {
        usage();
    }


    //merge(args.at(1), args.at(2), args.at(3));
    return app.exit();
}
