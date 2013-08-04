#ifndef GEOLOCATIONINTERPOLATOR_H
#define GEOLOCATIONINTERPOLATOR_H

#include "geolocationiterator.h"

class GeoLocationInterpolator {
public:
    GeoLocationInterpolator(const GeoLocationIterator &it);
    void reset();
    bool advance(double meters, double *latitude, double *longitude);

private:
    GeoLocationIterator m_it;
    struct {
        double lat;
        double lon;
    } prev, current, next;

};

#endif // GEOLOCATIONINTERPOLATOR_H
