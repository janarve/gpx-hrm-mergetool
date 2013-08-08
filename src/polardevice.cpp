#include "polardevice.h"

#ifdef HAVE_HRMCOM
#include <hrmcom.h>
#endif
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qdatetime.h>

#include <stdio.h>

#ifdef HAVE_HRMCOM
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
#endif

