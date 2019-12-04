#include "MapGraphicsView.h"

#include <QVBoxLayout>
#include <QTimer>
#include <QtDebug>
#include <cmath>
#include <QQueue>
#include <QSet>
#include <QWheelEvent>
#include <QCoreApplication>
#include <QThread>
#include <QMenu>

#include "guts/PrivateQGraphicsScene.h"
#include "guts/PrivateQGraphicsView.h"
#include "guts/Conversions.h"

MapGraphicsView::MapGraphicsView(MapGraphicsScene *scene, QWidget *parent) :
    QWidget(parent)
{
    //Setup the given scene and set the default zoomLevel to 3
    this->setScene(scene);
    _zoomLevel = 2;

    //The default drag mode allows us to drag the map around to move the view
    this->setDragMode(MapGraphicsView::ScrollHandDrag);

    //Start the timer that will cause the tiles to periodically move to follow the view
    QTimer * renderTimer = new QTimer(this);
    connect(renderTimer,
            SIGNAL(timeout()),
            this,
            SLOT(renderTiles()));
    renderTimer->start(200);
}

MapGraphicsView::~MapGraphicsView()
{
    qDebug() << this << "Destructing";
    //When we die, take all of our tile objects with us...
    foreach(MapTileGraphicsObject * tileObject, _tileObjects)
    {
        if (!_childScene.isNull())
            _childScene->removeItem(tileObject);
        delete tileObject;
    }
    _tileObjects.clear();

    if (!_tileSource.isNull())
    {
        //Find the tileSource's thread
        QPointer<QThread> tileSourceThread = _tileSource->thread();

        /*
         Clear the QSharedPointer to the tilesource. Unless there's a serious problem, we should be the
         last thing holding that reference and we expect it to be deleted
        */
        _tileSource.clear();

        //After the tilesource is deleted, we wait for the thread it was running in to shut down
        int count = 0;
        const int maxCount = 100;
        while (!tileSourceThread.isNull() && !tileSourceThread->wait(100))
        {
            //We have to process events while it's shutting down in case it uses signals/slots to shut down
            //Hint: it does
            QCoreApplication::processEvents(QEventLoop::ExcludeSocketNotifiers | QEventLoop::ExcludeUserInputEvents);
            if (++count == maxCount)
                break;
        }
    }
}

QPointF MapGraphicsView::center() const
{
    QPointF centerGeoPos = this->mapToScene(QPoint(this->width()/2,this->height()/2));
    return centerGeoPos;
}

void MapGraphicsView::centerOn(const QPointF &pos)
{
    if (_tileSource.isNull())
        return;

    //Find the QGraphicsScene coordinate of the position and then tell the childView to center there
    QPointF qgsPos = _tileSource->ll2qgs(pos,this->zoomLevel());

    _childView->centerOn(qgsPos);
}

void MapGraphicsView::centerOn(qreal longitude, qreal latitude)
{
    this->centerOn(QPointF(longitude,latitude));
}

void MapGraphicsView::centerOn(const MapGraphicsObject *item)
{
    if (item)
        this->centerOn(item->pos());
}

QPointF MapGraphicsView::mapToScene(const QPoint viewPos) const
{
    if (_tileSource.isNull())
    {
        qWarning() << "No tile source --- Transformation cannot work";
        return QPointF(0,0);
    }

    QPointF qgsScenePos = _childView->mapToScene(viewPos);

    //Convert from QGraphicsScene coordinates to geo (MapGraphicsScene) coordinates
    const quint8 zoom = this->zoomLevel();

    return _tileSource->qgs2ll(qgsScenePos,zoom);
}

MapGraphicsView::DragMode MapGraphicsView::dragMode() const
{
    return _dragMode;
}

void MapGraphicsView::setDragMode(MapGraphicsView::DragMode mode)
{
    _dragMode = mode;

    QGraphicsView::DragMode qgvDragMode;
    if (_dragMode == MapGraphicsView::NoDrag)
        qgvDragMode = QGraphicsView::NoDrag;
    else if (_dragMode == MapGraphicsView::ScrollHandDrag)
        qgvDragMode = QGraphicsView::ScrollHandDrag;
    else
        qgvDragMode = QGraphicsView::RubberBandDrag;

    if (_childView.isNull())
        return;

    _childView->setDragMode(qgvDragMode);
}

MapGraphicsScene *MapGraphicsView::scene() const
{
    return _scene;
}

void MapGraphicsView::setScene(MapGraphicsScene * scene)
{
    /*
      Create New Stuff
    */
    //Create a private QGraphicsScene that our (also private) QGraphicsView will use
    PrivateQGraphicsScene * childScene = new PrivateQGraphicsScene(scene,
                                                                   this,
                                                                   this);
    //The QGraphicsScene needs to know when our zoom level changes so it can notify objects
    connect(this,
            SIGNAL(zoomLevelChanged(quint8)),
            childScene,
            SLOT(handleZoomLevelChanged()));

    //Create a QGraphicsView that handles drawing for us
    PrivateQGraphicsView * childView = new PrivateQGraphicsView(childScene, this);
    connect(childView,
            SIGNAL(hadMouseDoubleClickEvent(QMouseEvent*)),
            this,
            SLOT(handleChildMouseDoubleClick(QMouseEvent*)));
    connect(childView,
            SIGNAL(hadMouseMoveEvent(QMouseEvent*)),
            this,
            SLOT(handleChildMouseMove(QMouseEvent*)));
    connect(childView,
            SIGNAL(hadMousePressEvent(QMouseEvent*)),
            this,
            SLOT(handleChildMousePress(QMouseEvent*)));
    connect(childView,
            SIGNAL(hadMouseReleaseEvent(QMouseEvent*)),
            this,
            SLOT(handleChildMouseRelease(QMouseEvent*)));
    connect(childView,
            SIGNAL(hadWheelEvent(QWheelEvent*)),
            this,
            SLOT(handleChildViewScrollWheel(QWheelEvent*)));
    connect(childView,
            SIGNAL(hadContextMenuEvent(QContextMenuEvent*)),
            this,
            SLOT(handleChildViewContextMenu(QContextMenuEvent*)));

    //Insert new stuff
    if (this->layout() != 0)
        delete this->layout();
    this->setLayout(new QVBoxLayout(this));
    this->layout()->addWidget(childView);
    // set resize anchor of child QGraphicsView to center so the center
    // position doesn't change when the view gets resized
    childView->setResizeAnchor(QGraphicsView::AnchorViewCenter);    


    //Delete old stuff if applicable
    if (!_childView.isNull())
        delete _childView;
    if (!_childScene.isNull())
        delete _childScene;


    //Set new stuff
    _childView = childView;
    _childScene = childScene;
    _scene = scene;

    this->resetQGSSceneSize();

    //Reset the drag mode for the new child view
    this->setDragMode(this->dragMode());
}

QSharedPointer<MapTileSource> MapGraphicsView::tileSource() const
{
    return _tileSource;
}

void MapGraphicsView::setTileSource(QSharedPointer<MapTileSource> tSource)
{
    _tileSource = tSource;

    if (!_tileSource.isNull())
    {
        //Create a thread just for the tile source
        QThread * tileSourceThread = new QThread();
        tileSourceThread->start();
        _tileSource->moveToThread(tileSourceThread);

        //Set it up so that the thread is destroyed when the tile source is!
        connect(_tileSource.data(),
                SIGNAL(destroyed()),
                tileSourceThread,
                SLOT(quit()));
        connect(tileSourceThread,
                SIGNAL(finished()),
                tileSourceThread,
                SLOT(deleteLater()));
    }

    //Update our tile displays (if any) about the new tile source
    foreach(MapTileGraphicsObject * tileObject, _tileObjects)
        tileObject->setTileSource(tSource);
}

quint8 MapGraphicsView::zoomLevel() const
{
    return _zoomLevel;
}

void MapGraphicsView::setZoomLevel(quint8 nZoom, ZoomMode zMode)
{
    if (_tileSource.isNull())
        return;

    //This stuff is for handling the re-centering upong zoom in/out
    const QPointF  centerGeoPos = this->mapToScene(QPoint(this->width()/2,this->height()/2));
    QPointF mousePoint = _childView->mapToScene(_childView->mapFromGlobal(QCursor::pos()));
    QRectF sceneRect = _childScene->sceneRect();
    const float xRatio = mousePoint.x() / sceneRect.width();
    const float yRatio = mousePoint.y() / sceneRect.height();
    const QPointF centerPos = _childView->mapToScene(QPoint(_childView->width()/2,_childView->height()/2));
    const QPointF offset = mousePoint - centerPos;

    //Change the zoom level
    nZoom = qMin(_tileSource->maxZoomLevel(),qMax(_tileSource->minZoomLevel(),nZoom));

    if (nZoom == _zoomLevel)
        return;

    _zoomLevel = nZoom;

    //Disable all tile display temporarily. They'll redisplay properly when the timer ticks
    foreach(MapTileGraphicsObject * tileObject, _tileObjects)
        tileObject->setVisible(false);

    //Make sure the QGraphicsScene is the right size
    this->resetQGSSceneSize();


    //Re-center the view where we want it
    sceneRect = _childScene->sceneRect();
    mousePoint = QPointF(sceneRect.width()*xRatio,
                         sceneRect.height()*yRatio) - offset;

    if (zMode == MouseZoom)
        _childView->centerOn(mousePoint);
    else
        this->centerOn(centerGeoPos);

    //Make MapGraphicsObjects update
    this->zoomLevelChanged(nZoom);
}

void MapGraphicsView::zoomIn(ZoomMode zMode)
{
    if (_tileSource.isNull())
        return;

    if (this->zoomLevel() < _tileSource->maxZoomLevel())
        this->setZoomLevel(this->zoomLevel()+1,zMode);
}

void MapGraphicsView::zoomOut(ZoomMode zMode)
{
    if (_tileSource.isNull())
        return;

    if (this->zoomLevel() > _tileSource->minZoomLevel())
        this->setZoomLevel(this->zoomLevel()-1,zMode);
}

//protected slot
void MapGraphicsView::handleChildMouseDoubleClick(QMouseEvent *event)
{
    event->setAccepted(false);
}

//protected slot
void MapGraphicsView::handleChildMouseMove(QMouseEvent *event)
{
    event->setAccepted(false);
}

//protected slot
void MapGraphicsView::handleChildMousePress(QMouseEvent *event)
{
    event->setAccepted(false);
}

//protected slot
void MapGraphicsView::handleChildMouseRelease(QMouseEvent *event)
{
    event->setAccepted(false);
}

//protected slot
void MapGraphicsView::handleChildViewContextMenu(QContextMenuEvent *event)
{
    event->setAccepted(false);
}

//protected slot
void MapGraphicsView::handleChildViewScrollWheel(QWheelEvent *event)
{
    event->setAccepted(true);

    this->setDragMode(MapGraphicsView::ScrollHandDrag);
    if (event->delta() > 0)
        this->zoomIn(MouseZoom);
    else
        this->zoomOut(MouseZoom);
}

//private slot
void MapGraphicsView::renderTiles()
{
    if (_scene.isNull())
    {
        qDebug() << "No MapGraphicsScene to render";
        return;
    }

    if (_tileSource.isNull())
    {
        qDebug() << "No MapTileSource to render";
        return;
    }


    //Layout the tile objects
    this->doTileLayout();
}

//protected
void MapGraphicsView::doTileLayout()
{
    //Calculate the center point and polygon of the viewport in QGraphicsScene coordinates
    const QPointF centerPointQGS = _childView->mapToScene(_childView->width()/2.0,
                                                          _childView->height()/2.0);
    QPolygon viewportPolygonQGV;
    viewportPolygonQGV << QPoint(0,0) << QPoint(0,_childView->height()) << QPoint(_childView->width(),_childView->height()) << QPoint(_childView->width(),0);

    const QPolygonF viewportPolygonQGS = _childView->mapToScene(viewportPolygonQGV);
    const QRectF boundingRect = viewportPolygonQGS.boundingRect();

    //We exaggerate the bounding rect for some purposes!
    QRectF exaggeratedBoundingRect = boundingRect;
    exaggeratedBoundingRect.setSize(boundingRect.size()*2.0);
    exaggeratedBoundingRect.moveCenter(boundingRect.center());

    //We'll mark tiles that aren't being displayed as free so we can use them
    QQueue<MapTileGraphicsObject *> freeTiles;

    QSet<QPointF> placesWhereTilesAre;
    foreach(MapTileGraphicsObject * tileObject, _tileObjects)
    {
        if (!tileObject->isVisible() || !exaggeratedBoundingRect.contains(tileObject->pos()))
        {
            freeTiles.enqueue(tileObject);
            tileObject->setVisible(false);
        }
        else
            placesWhereTilesAre.insert(tileObject->pos());
    }

    const quint16 tileSize = _tileSource->tileSize();
    const quint32 tilesPerRow = sqrt((long double)_tileSource->tilesOnZoomLevel(this->zoomLevel()));
    const quint32 tilesPerCol = tilesPerRow;

    const qint32 perSide = qMax(boundingRect.width()/tileSize,
                       boundingRect.height()/tileSize) + 3;
    const qint32 xc = qMax((qint32)0,
                     (qint32)(centerPointQGS.x() / tileSize) - perSide/2);
    const qint32 yc = qMax((qint32)0,
                     (qint32)(centerPointQGS.y() / tileSize) - perSide/2);
    const qint32 xMax = qMin((qint32)tilesPerRow,
                              xc + perSide);
    const qint32 yMax = qMin(yc + perSide,
                              (qint32)tilesPerCol);

    for (qint32 x = xc; x < xMax; x++)
    {
        for (qint32 y = yc; y < yMax; y++)
        {
            const QPointF scenePos(x*tileSize + tileSize/2,
                                   y*tileSize + tileSize/2);


            bool tileIsThere = false;
            if (placesWhereTilesAre.contains(scenePos))
                tileIsThere = true;

            if (tileIsThere)
                continue;

            //Just in case we're running low on free tiles, add one
            if (freeTiles.isEmpty())
            {
                MapTileGraphicsObject * tileObject = new MapTileGraphicsObject(tileSize);
                tileObject->setTileSource(_tileSource);
                _tileObjects.insert(tileObject);
                _childScene->addItem(tileObject);
                freeTiles.enqueue(tileObject);
            }
            //Get the first free tile and make it do its thing
            MapTileGraphicsObject * tileObject = freeTiles.dequeue();
            if (tileObject->pos() != scenePos)
                tileObject->setPos(scenePos);
            if (tileObject->isVisible() != true)
                tileObject->setVisible(true);
            tileObject->setTile(x,y,this->zoomLevel());
        }
    }

    //If we've got a lot of free tiles left over, delete some of them
    while (freeTiles.size() > 2)
    {
        MapTileGraphicsObject * tileObject = freeTiles.dequeue();
        _tileObjects.remove(tileObject);
        _childScene->removeItem(tileObject);
        delete tileObject;
    }

}

//protected
void MapGraphicsView::resetQGSSceneSize()
{
    if (_tileSource.isNull())
        return;

    //Make sure the size of our QGraphicsScene is correct
    const quint64 dimension = sqrt((long double)_tileSource->tilesOnZoomLevel(this->zoomLevel()))*_tileSource->tileSize();
    if (_childScene->sceneRect().width() != dimension)
        _childScene->setSceneRect(0,0,dimension,dimension);
}
