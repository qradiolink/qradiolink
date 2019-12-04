#include "Position.h"

#include "guts/Conversions.h"

#include <QtGlobal>
#include <cmath>

Position::Position()
{
    _lonLat = QPointF(0.0,0.0);
    _altitude = 0;
}

Position::Position(qreal longitude, qreal latitude, qreal altitude)
{
    _lonLat = QPointF(longitude,latitude);
    _altitude = altitude;
}

Position::Position(const QPointF &lonLat, qreal altitude)
{
    _lonLat = lonLat;
    _altitude = altitude;
}

Position::Position(const Position &other)
{
    _lonLat = other._lonLat;
    _altitude = other._altitude;
}

Position::~Position()
{
}

bool Position::operator ==(const Position &other) const
{
    return ((other._lonLat == this->_lonLat)
            && (other._altitude == this->_altitude));
}

bool Position::operator !=(const Position &other) const
{
    return !(*this == other);
}

Position &Position::operator =(const Position &other)
{
    if (&other != this)
    {
        _lonLat = other._lonLat;
        _altitude = other._altitude;
    }

    return *this;
}

qreal Position::longitude() const
{
    return _lonLat.x();
}

qreal Position::latitude() const
{
    return _lonLat.y();
}

qreal Position::altitude() const
{
    return _altitude;
}

QPointF Position::lonLat() const
{
    return _lonLat;
}

void Position::setLongitude(const qreal &longitude)
{
    _lonLat.setX(longitude);
}

void Position::setLatitude(const qreal &latitude)
{
    _lonLat.setY(latitude);
}

void Position::setAltitude(const qreal &altitude)
{
    _altitude = altitude;
}

qreal Position::flatDistanceEstimate(const Position &other) const
{
    const QVector2D offsetMeters = this->flatOffsetMeters(other);
    return offsetMeters.length();
}

QVector2D Position::flatOffsetMeters(const Position &dest) const
{
    const qreal avgLat = (this->latitude() + dest.latitude()) / 2.0;
    const qreal lonPerMeter = Conversions::degreesLonPerMeter(avgLat);
    const qreal latPerMeter = Conversions::degreesLatPerMeter(avgLat);

    const qreal lonDiffMeters = (dest.longitude() - this->longitude()) / lonPerMeter;
    const qreal latDiffMeters = (dest.latitude() - this->latitude()) / latPerMeter;

    return QVector2D(lonDiffMeters, latDiffMeters);
}

Position Position::flatOffsetToPosition(const QPointF& offset) const
{
    const qreal lonPerMeter = Conversions::degreesLonPerMeter(this->latitude());
    const qreal latPerMeter = Conversions::degreesLatPerMeter(this->latitude());

    return Position(this->longitude() + offset.x() * lonPerMeter,
                    this->latitude() + offset.y() * latPerMeter);
}

qreal Position::flatManhattanEstimate(const Position &other) const
{
    const QVector2D offsetMeters = this->flatOffsetMeters(other);
    return qAbs<qreal>(offsetMeters.x()) + qAbs<qreal>(offsetMeters.y());
}

qreal Position::angleTo(const Position &dest) const
{
    const QVector2D offsetMeters = this->flatOffsetMeters(dest);
    return atan2(offsetMeters.y(), offsetMeters.x());
}

//static
QVector3D Position::Position2ENU(const Position &refPos, const Position &pos)
{
    return Conversions::lla2enu(pos.latitude(),
                                pos.longitude(),
                                pos.altitude(),
                                refPos.latitude(),
                                refPos.longitude(),
                                refPos.altitude());
}

//static
Position Position::fromENU(const Position &refPos, const QVector3D &enu)
{
    Position cPos = Conversions::enu2lla(enu,
                                                      refPos.latitude(),
                                                      refPos.longitude(),
                                                      refPos.altitude());
    return cPos;
}

//Non-member method for streaming to qDebug
QDebug operator<<(QDebug dbg, const Position& pos)
{
    dbg.nospace() << "(" << QString::number(pos.longitude(),'g',10) << "," << QString::number(pos.latitude(),'g',10) << ")";

    return dbg.space();
}

//Non-member methods for serializing and de-serializing
QDataStream& operator<<(QDataStream& stream, const Position& pos)
{
    stream << pos.longitude() << pos.latitude() << pos.altitude();
    return stream;
}

QDataStream& operator>>(QDataStream& stream, Position& pos)
{
    qreal longitude;
    qreal latitude;
    qreal altitude;
    stream >> longitude;
    stream >> latitude;
    stream >> altitude;

    pos.setLongitude(longitude);
    pos.setLatitude(latitude);
    pos.setAltitude(altitude);

    return stream;
}

//Non-member method for hashing
uint qHash(const Position& pos)
{
    return pos.lonLat().x() + pos.lonLat().y() + pos.altitude();
}
