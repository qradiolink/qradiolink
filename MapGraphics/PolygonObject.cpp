#include "PolygonObject.h"

#include "guts/Conversions.h"
#include "CircleObject.h"

#include <QtDebug>
#include <QKeyEvent>

PolygonObject::PolygonObject(QPolygonF geoPoly, QColor fillColor, QObject *parent) :
    MapGraphicsObject(parent), _geoPoly(geoPoly), _fillColor(fillColor)
{
    this->setFlag(MapGraphicsObject::ObjectIsMovable);
    this->setFlag(MapGraphicsObject::ObjectIsSelectable,false);
    this->setFlag(MapGraphicsObject::ObjectIsFocusable);
    this->setGeoPoly(geoPoly);
}

PolygonObject::~PolygonObject()
{
    qDebug() << this << "destroying";
    foreach(MapGraphicsObject * circle, _editCircles)
        this->destroyEditCircle(circle);
    _editCircles.clear();

    foreach(MapGraphicsObject * circle, _addVertexCircles)
        this->destroyAddVertexCircle(circle);
    _addVertexCircles.clear();
}

//pure-virtual from MapGraphicsObject
QRectF PolygonObject::boundingRect() const
{
    QRectF latLonRect = _geoPoly.boundingRect();
    QPointF latLonCenter = latLonRect.center();
    Position latLonCenterPos(latLonCenter,0.0);
    Position topLeftPos(latLonRect.topLeft(),0.0);
    Position bottomRightPos(latLonRect.bottomRight(),0.0);

    QPointF topLeftENU = Conversions::lla2enu(topLeftPos,latLonCenterPos).toPointF();
    QPointF bottomRightENU = Conversions::lla2enu(bottomRightPos,latLonCenterPos).toPointF();

    return QRectF(topLeftENU,bottomRightENU);
}

//virtual from MapGraphicsObject
bool PolygonObject::contains(const QPointF &geoPos) const
{
    return _geoPoly.containsPoint(geoPos,
                                  Qt::OddEvenFill);
}

//pure-virtual from MapGraphicsObject
void PolygonObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing,true);

    QPolygonF enuPoly;

    Position latLonCenterPos(_geoPoly.boundingRect().center(),0);
    foreach(QPointF latLon, _geoPoly)
    {
        Position latLonPos(latLon,0.0);
        QPointF enu = Conversions::lla2enu(latLonPos,latLonCenterPos).toPointF();
        enuPoly << enu;
    }

    painter->setBrush(_fillColor);
    painter->drawPolygon(enuPoly);


    //Populate edit and add-vertex handles if necessary.
    //Is there a better place to do this? Most likely, yes.
    if (_editCircles.isEmpty())
    {
        //Create objects to edit the polygon!
        for (int i = 0; i < _geoPoly.size(); i++)
        {
            //Edit circles - to change the shape
            CircleObject * circle = this->constructEditCircle();
            circle->setPos(_geoPoly.at(i));
            _editCircles.append(circle);

            QPointF current = _geoPoly.at(i);
            QPointF next = _geoPoly.at((i+1) % _geoPoly.size());
            QPointF avg((current.x() + next.x())/2.0,
                        (current.y() + next.y())/2.0);

            //Add vertex circles - to add new vertices
            CircleObject * betweener = this->constructAddVertexCircle();
            betweener->setPos(avg);
            _addVertexCircles.append(betweener);
        }
    }
}

void PolygonObject::setPos(const QPointF & nPos)
{
    /*
      If the new position moved the object from the center of the bounding box made by the geo poly
      then we need to move the geo poly
    */
    if (nPos != _geoPoly.boundingRect().center())
    {
        const QPointF diff = nPos - this->pos();
        //_geoPoly.translate(diff);

        //We should also move our edit handles
        foreach(MapGraphicsObject * circle, _editCircles)
            circle->setPos(circle->pos() + diff);

        //And our "add vertex" handles
        this->fixAddVertexCirclePos();
    }

    MapGraphicsObject::setPos(nPos);

    //If this isn't here, we get TEARING when we edit our polygons
    this->posChanged();
}

QPolygonF PolygonObject::geoPoly() const
{
    return _geoPoly;
}

void PolygonObject::setGeoPoly(const QPolygonF &newPoly)
{
    if (newPoly == _geoPoly)
        return;

    _geoPoly = newPoly;

    foreach(MapGraphicsObject * obj, _editCircles)
        this->destroyEditCircle(obj);
    foreach(MapGraphicsObject * obj, _addVertexCircles)
        this->destroyAddVertexCircle(obj);
    _editCircles.clear();
    _addVertexCircles.clear();

    this->setPos(newPoly.boundingRect().center());
    this->polygonChanged(newPoly);
}

void PolygonObject::setFillColor(const QColor &color)
{
    if (_fillColor == color)
        return;

    _fillColor = color;
    this->redrawRequested();
}

//protected
//virtual from MapGraphicsObject
void PolygonObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    /*
      This method is a bit of a hack --- ideally contains() or some other method
      we "reimplement" from PrivateQGraphicsObject should take care of this, but it
      hasn't been working. This prevents the polygon from being grabbed when the user
      click inside the bounding box but outside of the actual polygon
    */
    const QPointF geoPos = event->scenePos();

    //If the geo point is within our geo polygon, we might accept it
    if (_geoPoly.containsPoint(geoPos,Qt::OddEvenFill))
        MapGraphicsObject::mousePressEvent(event);
    //Otherwise we ignore it
    else
        event->ignore();
}

//protected
//virtual from MapGraphicsObject
void PolygonObject::keyReleaseEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Delete))
    {
        this->deleteLater();
        event->accept();
    }
    else
        event->ignore();
}

//private slot
void PolygonObject::handleEditCirclePosChanged()
{
    QObject * sender = QObject::sender();
    if (sender == 0)
        return;
    CircleObject * circle = qobject_cast<CircleObject *>(sender);
    if (circle == 0)
        return;

    int index = _editCircles.indexOf(circle);
    if (index == -1)
        return;

    QPointF newPos = circle->pos();
    _geoPoly.replace(index,newPos);
    this->setPos(_geoPoly.boundingRect().center());

    //We need to update the positions of our "add vertex" controllers
    this->fixAddVertexCirclePos();

    //Emit a signal so everyone knows that the polygon changed
    this->polygonChanged(this->geoPoly());
}

//private slot
void PolygonObject::handleAddVertexCircleSelected()
{
    QObject * sender = QObject::sender();
    if (sender == 0)
        return;
    CircleObject * circle = qobject_cast<CircleObject *>(sender);
    if (circle == 0)
        return;

    //If the circle isn't selected, something is wrong
    if (!circle->isSelected())
        return;

    //Now that we know the circle was selected, just deselect it. We don't need it selected actually
    circle->setSelected(false);

    //Get the position at which we should add a vertex
    QPointF geoPos = circle->pos();

    //The index at which we should insert the new vertex
    int index = _addVertexCircles.indexOf(circle);
    if (index == -1)
        return;
    index++;

    //Put the vertex in the polygon
    _geoPoly.insert(index,geoPos);

    //Create a new "Edit Circle" and put it in the right spot
    CircleObject * editCircle = this->constructEditCircle();
    editCircle->setPos(geoPos);
    _editCircles.insert(index,editCircle);

    editCircle->setSelected(true);


    //Create a new "Add vertex" circle and put it in the right spot
    CircleObject * addVertexCircle = this->constructAddVertexCircle();
    QPointF current = _geoPoly.at(index-1);
    QPointF next = _geoPoly.at(index);
    QPointF avg((current.x() + next.x())/2.0,
                (current.y() + next.y())/2.0);
    addVertexCircle->setPos(avg);
    _addVertexCircles.insert(index,addVertexCircle);

    this->fixAddVertexCirclePos();

    //Emit a signal so everyone knows that the polygon changed
    this->polygonChanged(this->geoPoly());
}

//private slot
void PolygonObject::handleEditCircleDestroyed()
{
    QObject * sender = QObject::sender();
    if (sender == 0)
    {
        qWarning() << "Can't process desyroyed edit circle. Sender is null";
        return;
    }
    CircleObject * circle = (CircleObject *) sender;

    int index = _editCircles.indexOf(circle);
    if (index == -1)
    {
        qWarning() << "Can't process destroyed edit circle. Not contained in edit circle list";
        return;
    }

    _geoPoly.remove(index);
    _editCircles.removeAt(index);
    _addVertexCircles.takeAt(index)->deleteLater();

    this->fixAddVertexCirclePos();
    this->redrawRequested();
    this->setPos(_geoPoly.boundingRect().center());
}

//private
void PolygonObject::fixAddVertexCirclePos()
{
    for (int i = 0; i < _geoPoly.size(); i++)
    {
        QPointF current = _geoPoly.at(i);
        QPointF next = _geoPoly.at((i+1) % _geoPoly.size());
        QPointF avg((current.x() + next.x())/2.0,
                    (current.y() + next.y())/2.0);
        _addVertexCircles.at(i)->setPos(avg);
    }
}

//private
CircleObject *PolygonObject::constructEditCircle()
{
    CircleObject * toRet = new CircleObject(8);
    connect(toRet,
            SIGNAL(posChanged()),
            this,
            SLOT(handleEditCirclePosChanged()));
    connect(toRet,
            SIGNAL(destroyed()),
            this,
            SLOT(handleEditCircleDestroyed()));

    this->newObjectGenerated(toRet);
    return toRet;
}

//private
void PolygonObject::destroyEditCircle(MapGraphicsObject *obj)
{
    disconnect(obj,
               SIGNAL(posChanged()),
               this,
               SLOT(handleEditCirclePosChanged()));
    disconnect(obj,
               SIGNAL(destroyed()),
               this,
               SLOT(handleEditCircleDestroyed()));
    obj->deleteLater();
}

//private
CircleObject *PolygonObject::constructAddVertexCircle()
{
    CircleObject * toRet = new CircleObject(3,
                                            true,
                                            QColor(100,100,100,255));
    toRet->setFlag(MapGraphicsObject::ObjectIsMovable,false);
    connect(toRet,
            SIGNAL(selectedChanged()),
            this,
            SLOT(handleAddVertexCircleSelected()));

    this->newObjectGenerated(toRet);
    toRet->setToolTip("Single-click (don't drag!) to add vertex.");
    return toRet;
}

//private
void PolygonObject::destroyAddVertexCircle(MapGraphicsObject *obj)
{
    disconnect(obj,
               SIGNAL(selectedChanged()),
               this,
               SLOT(handleAddVertexCircleSelected()));
    obj->deleteLater();
}
