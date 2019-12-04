#include "LineObject.h"

#include "guts/Conversions.h"
#include <QtGlobal>

LineObject::LineObject(const Position &endA,
                       const Position &endB,
                       qreal thickness,
                       MapGraphicsObject *parent) :
    MapGraphicsObject(false, parent),
    _a(endA), _b(endB)
{
    _thickness = qBound<qreal>(0.0, thickness, 5.0);
    this->updatePositionFromEndPoints();
}

LineObject::~LineObject()
{
}

//pure-virtual from MapGraphicsObject
QRectF LineObject::boundingRect() const
{
    const qreal avgLat = (_a.latitude() + _b.latitude()) / 2.0;
    const qreal lonPerMeter = Conversions::degreesLonPerMeter(avgLat);
    const qreal latPerMeter = Conversions::degreesLatPerMeter(avgLat);

    const qreal widthLon = qAbs<qreal>(_a.longitude() - _b.longitude());
    const qreal heightlat = qAbs<qreal>(_a.latitude() - _b.latitude());

    const qreal widthMeters = qMax<qreal>(widthLon / lonPerMeter, 5.0);
    const qreal heightMeters = qMax<qreal>(heightlat / latPerMeter, 5.0);

    const QRectF toRet(-1.0 * widthMeters,
                       -1.0 * heightMeters,
                       2.0 * widthMeters,
                       2.0 * heightMeters);

    return toRet;
}

//pure-virtual from MapGraphicsObject
void LineObject::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen pen = painter->pen();
    pen.setWidthF(_thickness);
    painter->setPen(pen);

    const qreal avgLat = (_a.latitude() + _b.latitude()) / 2.0;
    const qreal lonPerMeter = Conversions::degreesLonPerMeter(avgLat);
    const qreal latPerMeter = Conversions::degreesLatPerMeter(avgLat);

    const QPointF center = this->pos();
    const QPointF offsetA = _a.lonLat() - center;
    const QPointF offsetB = _b.lonLat() - center;

    const QPointF metersA(offsetA.x() / lonPerMeter,
                          offsetA.y() / latPerMeter);
    const QPointF metersB(offsetB.x() / lonPerMeter,
                          offsetB.y() / latPerMeter);

    painter->drawLine(metersA, metersB);
}

qreal LineObject::thickness() const
{
    return _thickness;
}

void LineObject::setThickness(qreal nThick)
{
    _thickness = qBound<qreal>(0.0, nThick, 5.0);
    this->redrawRequested();
}

//public slot
void LineObject::setEndPointA(const Position &pos)
{
    _a = pos;
    this->updatePositionFromEndPoints();
    this->redrawRequested();
}

//public slot
void LineObject::setEndPointB(const Position &pos)
{
    _b = pos;
    this->updatePositionFromEndPoints();
    this->redrawRequested();
}

//public slot
void LineObject::setEndPoints(const Position &a,
                              const Position &b)
{
    _a = a;
    _b = b;
    this->updatePositionFromEndPoints();
    this->redrawRequested();
}

//private slot
void LineObject::updatePositionFromEndPoints()
{
    const qreal avgLon = (_a.longitude() + _b.longitude()) / 2.0;
    const qreal avgLat = (_a.latitude() + _b.latitude()) / 2.0;
    this->setPos(QPointF(avgLon, avgLat));
}
