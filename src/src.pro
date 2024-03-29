######################################################################
# Automatically generated by qmake (2.01a) to 26. mai 00:01:48 2011
######################################################################

TEMPLATE = app
QT += core gui
TARGET = hrmgpx
DESTDIR = bin
#DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += main.cpp \
    gpxparser.cpp \
    hrmparser.cpp \
    gpssample.cpp \ 
    geo.cpp \
    geolocationinterpolator.cpp \
    geolocationiterator.cpp

CONFIG += console

HEADERS += \
    gpxparser.h \
    hrmparser.h \
    gpssample.h \
    geo.h \
    geolocationinterpolator.h \
    geolocationiterator.h

exists(hrmcom/hrmcom.pri) {
    include(hrmcom/hrmcom.pri)
    DEFINES *= HAVE_HRMCOM
    SOURCES *= polardevice.cpp
    HEADERS *= polardevice.h
} else {
    message("Polar hrmcom library not found, disabling Polar HRM monitor support")
}
