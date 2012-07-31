#ifndef GPXPARSER_H
#define GPXPARSER_H

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QXmlStreamReader>
#include "gpssample.h"

bool loadGPX(SampleData *sampleData, QIODevice *device);
bool saveGPX(const SampleData &sampleData, QIODevice *device);

#endif // GPXPARSER_H
