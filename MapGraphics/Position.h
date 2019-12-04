#ifndef POSITION_H
#define POSITION_H

#include <QPointF>
#include <QtDebug>
#include <QVector3D>
#include <QVector2D>

#include "MapGraphics_global.h"

class MAPGRAPHICSSHARED_EXPORT Position
{
public:
    Position();
    Position(qreal longitude, qreal latitude, qreal altitude = 0.0);
    Position(const QPointF& lonLat, qreal altitude = 0.0);
    Position(const Position& other);
    ~Position();

    bool operator ==(const Position& other) const;
    bool operator !=(const Position& other) const;
    Position& operator =(const Position& other);

    qreal longitude() const;
    qreal latitude() const;
    qreal altitude() const;
    QPointF lonLat() const;

    void setLongitude(const qreal& longitude);
    void setLatitude(const qreal& latitude);
    void setAltitude(const qreal& altitude);

    qreal flatDistanceEstimate(const Position& other) const;
    QVector2D flatOffsetMeters(const Position& dest) const;
    Position flatOffsetToPosition(const QPointF &offset) const;
    qreal flatManhattanEstimate(const Position& other) const;
    qreal angleTo(const Position& dest) const;

    static QVector3D Position2ENU(const Position& refPos, const Position& pos);
    static Position fromENU(const Position& refPos, const QVector3D& enu);

private:
    QPointF _lonLat;
    qreal _altitude;
};

//Non-member method for streaming to qDebug
MAPGRAPHICSSHARED_EXPORT QDebug operator<<(QDebug dbg, const Position& pos);

//Non-member methods for serializing and de-serializing
MAPGRAPHICSSHARED_EXPORT QDataStream& operator<<(QDataStream& stream, const Position& pos);
MAPGRAPHICSSHARED_EXPORT QDataStream& operator>>(QDataStream& stream, Position& pos);

//Non-member method for hashing

MAPGRAPHICSSHARED_EXPORT uint qHash(const Position& pos);

#endif // POSITION_H
