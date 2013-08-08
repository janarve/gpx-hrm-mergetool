#ifndef POLARDEVICE_H
#define POLARDEVICE_H
#include <qt_windows.h>

#ifdef HAVE_HRMCOM
int readHRMData(HWND hWnd);
#endif
#endif // POLARDEVICE_H
