#include "geolocationiterator.h"
#include "gpssample.h"

GeoLocationIterator::GeoLocationIterator(const SampleData *samples) : m_currentIndex(-1), m_samples(samples) {

}

bool GeoLocationIterator::next(double *lat, double *lon)
{
    if (m_currentIndex >= m_samples->count())
        return false;
    ++m_currentIndex;
    return peek(lat, lon);  // will fail if we got beyond the last
}

bool GeoLocationIterator::peek(double *lat, double *lon) const
{
    if (m_currentIndex == -1 || m_currentIndex >= m_samples->count())
        return false;
    GpsSample s = m_samples->at(m_currentIndex);
    *lat = s.lat;
    *lon = s.lon;
    return true;
}

void GeoLocationIterator::reset()
{
    m_currentIndex = -1;
}

bool GeoLocationIterator::atEnd() const
{
    return m_currentIndex >= m_samples->count();
}

bool GeoLocationIterator::atBeginning() const
{
    return m_currentIndex == -1;
}
