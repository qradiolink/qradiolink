#include "CircleObject.h"

#include <QtDebug>
#include <QKeyEvent>

CircleObject::CircleObject(qreal radius,bool sizeIsZoomInvariant, QColor fillColor, MapGraphicsObject *parent) :
    MapGraphicsObject(sizeIsZoomInvariant,parent), _fillColor(fillColor)
{
    _radius = qMax<qreal>(radius,0.01);

    this->setFlag(MapGraphicsObject::ObjectIsSelectable);
    this->setFlag(MapGraphicsObject::ObjectIsMovable);
    this->setFlag(MapGraphicsObject::ObjectIsFocusable);
}

CircleObject::~CircleObject()
{
}

QRectF CircleObject::boundingRect() const
{
    return QRectF(-1*_radius,
                  -1*_radius,
                  2*_radius,
                  2*_radius);
}

void CircleObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    painter->setRenderHint(QPainter::Antialiasing,true);
    painter->setBrush(_fillColor);
    painter->drawEllipse(QPointF(0,0),
                         _radius,
                         _radius);
}

qreal CircleObject::radius() const
{
    return _radius;
}

void CircleObject::setRadius(qreal radius)
{
    _radius = radius;
    this->redrawRequested();
}

//protected
//virtual from MapGraphicsObject
void CircleObject::keyReleaseEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Delete))
    {
        this->deleteLater();
        event->accept();
    }
    else
        event->ignore();
}
