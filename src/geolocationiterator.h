#ifndef GEOLOCATIONITERATOR_H
#define GEOLOCATIONITERATOR_H

class SampleData;

class GeoLocationIterator {
public:
    GeoLocationIterator(const SampleData *samples);
    bool next(double *lat, double *lon);
    bool peek(double *lat, double *lon) const;
    void reset();
    bool atEnd() const;
    bool atBeginning() const;
private:
    int m_currentIndex;
    const SampleData *m_samples;
};


#endif // GEOLOCATIONITERATOR_H
