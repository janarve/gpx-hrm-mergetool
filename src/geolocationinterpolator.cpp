#include "geolocationinterpolator.h"
#include "geolocationiterator.h"
#include "geo.h"
#include <QtCore/qglobal.h>

#include <stdio.h>

static const double InvalidGeoPos = 400.0;


GeoLocationInterpolator::GeoLocationInterpolator(const GeoLocationIterator &it)
    : m_it(it)
{
    prev.lat = InvalidGeoPos;
    next.lat = InvalidGeoPos;
}

void GeoLocationInterpolator::reset() {
    m_it.reset();
    prev.lat = InvalidGeoPos;
    next.lat = InvalidGeoPos;
}

bool GeoLocationInterpolator::advance(double meters, double *latitude, double *longitude)
{
    if (prev.lat == InvalidGeoPos) {
        if (!m_it.next(&prev.lat, &prev.lon))
            return false;
        current = prev;
        if (!m_it.next(&next.lat, &next.lon))
            return false;
    }

    double gpxDist = 1000 * haversineDistance(current.lat, current.lon, next.lat, next.lon);
    if (meters > gpxDist) {
        while (meters > gpxDist) {
            printf("m: %f, gpxDist:%f\n", meters, gpxDist);
            double lat, lon;
            if (m_it.next(&lat, &lon)) {
                prev = next;
                next.lat = lat;
                next.lon = lon;
            }
            meters -= gpxDist;
            gpxDist = 1000 * haversineDistance(prev.lat, prev.lon, next.lat, next.lon);
        }
        current = prev;
    }
    Q_ASSERT(meters <= gpxDist);  // meters might be 0
    double progress = meters/gpxDist;
    current.lat = current.lat * (1.0 - progress) + next.lat * progress;
    current.lon = current.lon * (1.0 - progress) + next.lon * progress;
    *latitude = current.lat;
    *longitude = current.lon;
    return true;
}
