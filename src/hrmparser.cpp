#include "hrmparser.h"

float HRMParser::startAltitude() const
{
    if (hrms.count() > 0)
        return hrms.first().altitude;
    return -1.0;
}

float HRMParser::endAltitude() const
{
    if (hrms.count() > 0)
        return hrms.last().altitude;
    return -1.0;
}

qint64 HRMParser::startTime() const
{
    if (hrms.count() > 0)
        return hrms.first().time;
    return -1.0;
}

qint64 HRMParser::endTime() const
{
    if (hrms.count() > 0)
        return hrms.last().time;
    return -1.0;
}

bool HRMParser::parse()
{
    QFile file(m_fileName);
    if (file.open(QIODevice::ReadOnly)) {
        int timeOffset = 0;
        //bool speed;
        //bool cadence;
        //bool altitude;
        //bool power;
        // power left right balance
        // power pedalling index
        //bool hrcc;
        //bool unit;
        //bool airpressure;

        m_lineNumber = 1;
        m_startTime = 0;
        do {
            QByteArray line = file.readLine(1000).trimmed();
            Section section;
            if (line.count() > 0 && line.at(0) == '[') {
                if (line == "[Params]") {
                    section = Params;
                } else if (line == "[HRData]") {
                    timeOffset = 0;
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
                                ss = line.mid(19,1);
                                int ms = ss.toInt();
                                if (ms > 0) {
                                    QTime midnight(0,0);
                                    int msecs = midnight.msecsTo(time) + ms;
                                    m_startTime += msecs;
                                }
                            } else {
                                error("Params.StartTime is invalid");
                            }
                        } else if (line.indexOf("SMode=") == 0) {
                            QByteArray ss = line.mid(5);
                            //bool speed = (ss.at(0) == '1');
                            //bool cadence = (ss.at(1) == '1');
                            //bool altitude = (ss.at(2) == '1');
                            //bool power = (ss.at(3) == '1');
                            // power left right balance
                            // power pedalling index
                            //bool hrcc = (ss.at(6) == '1');  //0 = HR data only
                                                            //1 = HR + cycling data
                            //bool unit = (ss.at(7) == '1');  //0 = Euro (km, km/h, m, �C)
                                                            //1 = US (miles, mph, ft, �F)

                            //bool airpressure = (ss.count() < 8 ? false : (ss.at(8) == '1'));  //i) Air pressure (0=off, 1=on) &
                            /*
                            bits: abcdefghi
                            Data type parameters
                            a) Speed (0=off, 1=on)
                            b) Cadence (0=off, 1=on)
                            c) Altitude (0=off, 1=on)
                            d) Power (0=off, 1=on)
                            e) Power Left Right Balance (0=off, 1=on)
                            f) Power Pedalling Index (0=off, 1=on)
                            g) HR/CC data

                            h) US / Euro unit
                            0 = Euro (km, km/h, m, �C)
                            1 = US (miles, mph, ft, �F)
                            All distance, speed, altitude and temperature values depend on US/Euro unit
                            selection (km / miles, km/h / mph, m / ft, �C / �F).
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
                            HRMEntry hrm;
                            QTime time = m_time.addSecs(timeOffset);
                            Time t = Time::fromQTime(time);

                            QStringList list = strLine.split(QLatin1Char(' '), QString::SkipEmptyParts);
                            hrm.time = t;
                            hrm.hr = list.at(0).toFloat();
                            hrm.speed = list.at(1).toFloat();
                            hrm.altitude = list.at(2).toFloat();
                            hrms << hrm;
                            timeOffset+=60; //###
                        }
                    break;}
                    default:
                    break;
                }
            }
            ++m_lineNumber;
        }  while (!file.atEnd());
    } else {
        error("Could not open hrm file");
    }
    qDebug() << hrms.count();
    return true;
}
