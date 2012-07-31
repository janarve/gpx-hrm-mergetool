#include "hrmparser.h"

qint64 HRMReader::startTime() const
{
    return m_startTime;
}

qint64 HRMReader::endTime() const
{
    return m_startTime + m_length;
}

bool HRMReader::read(SampleData *sampleData)
{
    QFile file(m_fileName);
    if (file.open(QIODevice::ReadOnly)) {
        bool speed = false;
        //bool cadence;
        bool altitude = false;
        //bool power;
        // power left right balance
        // power pedalling index
        bool hrcc = false;
        bool unitIsUS = false;
        //bool airpressure;

        m_lineNumber = 1;
        m_startTime = 0;
        qint64 time = -1;
        do {
            QByteArray line = file.readLine(1000).trimmed();
            Section section;
            if (line.count() > 0 && line.at(0) == '[') {
                if (line == "[Params]") {
                    section = Params;
                } else if (line == "[HRData]") {
                    time = m_startTime;
                    section = HRData;
                } else {
                    section = None;
                }
            } else {
                switch (section) {
                    case Params:
                        if (line.indexOf("Interval=") == 0) {
                            QByteArray ss = line.mid(9);
                            bool ok;
                            int interval = ss.toInt(&ok);
                            if (ok)
                                m_interval = interval;
                        } else if (line.indexOf("Date=") == 0) {
                            QByteArray ss = line.mid(5);
                            QDate date = QDate::fromString(ss, QLatin1String("yyyyMMdd"));
                            if (!date.isValid()) {
                                error("Params.Date is invalid");
                            }
                            QDateTime dt(date);
                            m_startTime += dt.toMSecsSinceEpoch();
                        } else if (line.indexOf("StartTime=") == 0) {
                            QByteArray ss = line.mid(10, 8);
                            QTime time = QTime::fromString(ss, QLatin1String("hh:mm:ss"));
                            if (time.isValid()) {
                                ss = line.mid(18);  // ".79"
                                float sec = ss.toFloat();
                                int ms = qRound(sec * 1000);
                                if (ms >= 0) {
                                    QTime midnight(0,0);
                                    int msecs = midnight.msecsTo(time) + ms;
                                    m_startTime += msecs;
                                }
                            } else {
                                error("Params.StartTime is invalid");
                            }
                        } else if (line.indexOf("Length=") == 0) {
                            QByteArray ss = line.mid(7, 8);
                            QTime time = QTime::fromString(ss, QLatin1String("hh:mm:ss"));
                            if (time.isValid()) {
                                ss = line.mid(15);
                                float sec = ss.toFloat();
                                int ms = qRound(sec * 1000);
                                if (ms >= 0) {
                                    QTime midnight(0,0);
                                    int msecs = midnight.msecsTo(time) + ms;
                                    m_length = msecs;
                                }
                            } else {
                                error("Params.StartTime is invalid");
                            }
                        } else if (line.indexOf("SMode=") == 0) {
                            QByteArray ss = line.mid(6);
                            speed = (ss.at(0) == '1');
                            //bool cadence = (ss.at(1) == '1');
                            altitude = (ss.at(2) == '1');
                            //bool power = (ss.at(3) == '1');
                            // power left right balance
                            // power pedalling index
                            if (ss.at(6) == '1')
                                sampleData->metaData.activity = SampleData::Cycling;  //0 = HR data only
                                                            //1 = HR + cycling data
                            unitIsUS = (ss.at(7) == '1');  //0 = Euro (km, km/h, m, °C)
                                                            //1 = US (miles, mph, ft, °F)

                            //bool airpressure = (ss.count() < 8 ? false : (ss.at(8) == '1'));  //i) Air pressure (0=off, 1=on) &
                            /*
                            bits: abcdefghi
                            Data type parameters

                            [version >= 1.06]
                            a) Speed (0=off, 1=on)
                            b) Cadence (0=off, 1=on)
                            c) Altitude (0=off, 1=on)
                            d) Power (0=off, 1=on)
                            e) Power Left Right Balance (0=off, 1=on)
                            f) Power Pedalling Index (0=off, 1=on)
                            g) HR/CC data

                            h) US / Euro unit
                            0 = Euro (km, km/h, m, °C)
                            1 = US (miles, mph, ft, °F)
                            All distance, speed, altitude and temperature values depend on US/Euro unit
                            selection (km / miles, km/h / mph, m / ft, °C / °F).

                            [version >= 1.07]
                            i) Air pressure (0=off, 1=on) &
                            */
                        }
                    break;
                    case HRData: {
                        QRegExp rx;
                        rx.setPattern(QLatin1String("\\d+\\s+\\d+\\s+\\d+"));
                        rx.setPatternSyntax(QRegExp::W3CXmlSchema11);
                        QString strLine(line.simplified());
                        if (rx.exactMatch(strLine)) {
                            /*
                            hrm spd alt
                            82  0   204
                            151 184 207
                            */
                            GpsSample sample;
                            QStringList list = strLine.split(QLatin1Char(' '), QString::SkipEmptyParts);
                            sample.time = time;
                            sample.hr = list.at(0).toInt();
                            sample.speed = list.at(1).toFloat();
                            sample.ele = list.at(2).toFloat();
                            sampleData->append(sample);
                            time += m_interval * 1000;
                        }
                    break;}
                    default:
                    break;
                }
            }
            ++m_lineNumber;
        }  while (!file.atEnd());
        
        /* Since the HRM monitor will store data in fixed intervals, the last sample
           will usually be before the monitor was stopped, thus if the Interval is 
           60 seconds, it might be up to one minute off.
           We then add another "sample" corresponding to the time the monitor was stopped.
           This sample is just a copy of the previous one, but with time adjusted.
        */
        GpsSample sample = sampleData->last();
        if (sample.time < m_startTime + m_length) {
            sample.time = m_startTime + m_length;
            sampleData->append(sample);
        }
    } else {
        error("Could not open hrm file");
    }
    return true;
}
