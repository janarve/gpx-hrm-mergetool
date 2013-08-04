#include "geo.h"

#include <cmath>
#include <float.h>
#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

inline double toRad(double deg)
{
    return deg * M_PI/180.0;
}

double haversineDistance(double lat1, double lon1, double lat2, double lon2)
{
    static const double R = 6371.0;   //km
    double dLat = toRad(lat2 - lat1);
    double dLon = toRad(lon2 - lon1);
    lat1 = toRad(lat1);
    lat2 = toRad(lat2);

    double sinLat = sin(dLat/2.0);
    double sinLon = sin(dLon/2.0);
    double a = sinLat * sinLat +
               sinLon * sinLon * cos(lat1) * cos(lat2);

    // In case of imprecision, make sure a is in the range [0..1]
    if (a >= 1.0)
        a = 1.0;
    if (a < 0)
        a = 0;
    double c = 2 * atan2(sqrt(a), sqrt(1.0 - a));

    return R * c;
}

/*
from: http://www.movable-type.co.uk/scripts/latlong.html
javascript:
function toRad(deg)
{
    return deg * Math.PI/180;
}

var R = 6371; // km
var dLat = toRad(lat2-lat1);
var dLon = toRad(lon2-lon1);
var lat1 = toRad(lat1);
var lat2 = toRad(lat2);

var a = Math.sin(dLat/2) * Math.sin(dLat/2) +
        Math.sin(dLon/2) * Math.sin(dLon/2) * Math.cos(lat1) * Math.cos(lat2);
var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
var d = R * c;

WScript.Echo("Distance:" + d);
*/
