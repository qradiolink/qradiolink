#include "MapGraphicsObject.h"

#include <QtDebug>
#include <QKeyEvent>
#include <QTimer>

MapGraphicsObject::MapGraphicsObject(bool sizeIsZoomInvariant, MapGraphicsObject *parent) :
    _sizeIsZoomInvariant(sizeIsZoomInvariant), _constructed(false)
{
    //Set default properties and the parent that was passed as argument
    _enabled = true;
    _opacity = 1.0;
    this->setParent(parent);
    _pos = QPointF(0.0,0.0);
    _rotation = 0.0;
    _visible = true;
    _zValue = 0.0;
    _selected = false;


    /*
     * When we get back to the event loop, mark us as constructed.
     * This is a hack so that we can set properties of child objects in their constructors
    */
    QTimer::singleShot(1, this, SLOT(setConstructed()));
}

MapGraphicsObject::~MapGraphicsObject()
{
}

bool MapGraphicsObject::sizeIsZoomInvariant() const
{
    return _sizeIsZoomInvariant;
}

bool MapGraphicsObject::contains(const QPointF &geoPos) const
{
    QRectF geoRect = this->boundingRect();
    return geoRect.contains(geoPos);
}

bool MapGraphicsObject::enabled() const
{
    return _enabled;
}

void MapGraphicsObject::setEnabled(bool nEnabled)
{
    _enabled = nEnabled;
    if (_constructed)
        this->enabledChanged();
    else
        QTimer::singleShot(1, this, SIGNAL(enabledChanged()));
}

qreal MapGraphicsObject::opacity() const
{
    return _opacity;
}

void MapGraphicsObject::setOpacity(qreal nOpacity)
{
    _opacity = nOpacity;
    if (_constructed)
        this->opacityChanged();
    else
        QTimer::singleShot(1, this, SIGNAL(opacityChanged()));
}

MapGraphicsObject *MapGraphicsObject::parent() const
{
    return _parent;
}

void MapGraphicsObject::setParent(MapGraphicsObject * nParent)
{
    _parent = nParent;
    if (_constructed)
        this->parentChanged();
    else
        QTimer::singleShot(1, this, SIGNAL(parentChanged()));
}

const QPointF &MapGraphicsObject::pos() const
{
    return _pos;
}

void MapGraphicsObject::setPos(const QPointF & nPos)
{
    if (nPos == _pos)
        return;
    _pos = nPos;

    if (_constructed)
        this->posChanged();
    else
        QTimer::singleShot(1, this, SIGNAL(posChanged()));
}

qreal MapGraphicsObject::rotation() const
{
    return _rotation;
}

void MapGraphicsObject::setRotation(qreal nRotation)
{
    if (nRotation == _rotation)
        return;
    _rotation = nRotation;

    if (_constructed)
        this->rotationChanged();
    else
        QTimer::singleShot(1, this, SIGNAL(rotationChanged()));
}

bool MapGraphicsObject::visible() const
{
    return _visible;
}

void MapGraphicsObject::setVisible(bool nVisible)
{
    if (nVisible == _visible)
        return;
    _visible = nVisible;

    if (_constructed)
        this->visibleChanged();
    else
        QTimer::singleShot(1, this, SIGNAL(visibleChanged()));
}

qreal MapGraphicsObject::longitude() const
{
    return _pos.x();
}

void MapGraphicsObject::setLongitude(qreal nLongitude)
{
    this->setPos(QPointF(nLongitude,this->pos().y()));
}

qreal MapGraphicsObject::latitude() const
{
    return _pos.y();
}

void MapGraphicsObject::setLatitude(qreal nLatitude)
{
    this->setPos(QPointF(this->pos().x(),nLatitude));
}

qreal MapGraphicsObject::zValue() const
{
    return _zValue;
}

void MapGraphicsObject::setZValue(qreal nZValue)
{
    _zValue = nZValue;

    if (_constructed)
        this->zValueChanged();
    else
        QTimer::singleShot(1, this, SIGNAL(zValueChanged()));
}

bool MapGraphicsObject::isSelected() const
{
    return _selected;
}

void MapGraphicsObject::setSelected(bool sel)
{
    if (_selected == sel)
        return;
    _selected = sel;

    if (_constructed)
        this->selectedChanged();
    else
        QTimer::singleShot(1, this, SIGNAL(selectedChanged()));
}

QString MapGraphicsObject::toolTip() const
{
    return _toolTip;
}

void MapGraphicsObject::setToolTip(const QString &toolTip)
{
    _toolTip = toolTip;
    this->toolTipChanged(toolTip);
}

void MapGraphicsObject::setFlag(MapGraphicsObject::MapGraphicsObjectFlag flag, bool enabled)
{
    if (enabled)
        _flags = _flags | flag;
    else
        _flags = _flags & (~flag);

    this->flagsChanged();
}

void MapGraphicsObject::setFlags(MapGraphicsObject::MapGraphicsObjectFlags flags)
{
    _flags = flags;

    if (_constructed)
        this->flagsChanged();
    else
        QTimer::singleShot(1, this, SIGNAL(flagsChanged()));
}

MapGraphicsObject::MapGraphicsObjectFlags MapGraphicsObject::flags() const
{
    return _flags;
}

//protected
void MapGraphicsObject::contextMenuEvent(QGraphicsSceneContextMenuEvent * event)
{
    event->ignore();
}

//protected
QVariant MapGraphicsObject::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    Q_UNUSED(change)
    return value;
}

//protected
void MapGraphicsObject::keyPressEvent(QKeyEvent *event)
{
    event->ignore();
}

//protected
void MapGraphicsObject::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
}

//protected
void MapGraphicsObject::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
}

//protected
void MapGraphicsObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
}

//protected
void MapGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //The default is to accept this if we are selectable and/or movable. Otherwise, ignore!
    if (this->flags() & MapGraphicsObject::ObjectIsMovable
            ||this->flags() & MapGraphicsObject::ObjectIsSelectable)
    {
        event->accept();
        if (this->flags() & MapGraphicsObject::ObjectIsFocusable)
            this->keyFocusRequested();
    }
    else
        event->ignore();
}

//protected
void MapGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
}

//protected
void MapGraphicsObject::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    event->ignore();
}


//private slot
void MapGraphicsObject::setConstructed()
{
    _constructed = true;
}
