#ifndef GPXPARSER_H
#define GPXPARSER_H

#include <QtCore/qstring.h>
#include <QtCore/qvector.h>
#include "gpssample.h"

class GpxParser
{
public:
    GpxParser(const QString &fileName);
    bool parse();
    float startAltitude() const;
    float endAltitude() const;
    qint64 startTime() const;
    qint64 endTime() const;

public:
    SampleData samples;
    QString m_fileName;
};

#endif // GPXPARSER_H
