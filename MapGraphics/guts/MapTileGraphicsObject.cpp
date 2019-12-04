#include "MapTileGraphicsObject.h"

#include <QPainter>
#include <QtDebug>

MapTileGraphicsObject::MapTileGraphicsObject(quint16 tileSize)
{
    this->setTileSize(tileSize);
    _tile = 0;
    _tileX = 0;
    _tileY = 0;
    _tileZoom = 0;
    _initialized = false;
    _havePendingRequest = false;

    //Default z-value is important --- used in MapGraphicsView
    this->setZValue(-1.0);
}

MapTileGraphicsObject::~MapTileGraphicsObject()
{
    if (_tile != 0)
    {
        delete _tile;
        _tile = 0;
    }
}

QRectF MapTileGraphicsObject::boundingRect() const
{
    const quint16 size = this->tileSize();
    return QRectF(-1 * size / 2.0,
                  -1 * size / 2.0,
                  size,
                  size);
}

void MapTileGraphicsObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    //If we've got a tile, draw it. Otherwise, show a loading or "No tile source" message
    if (_tile != 0)
        painter->drawPixmap(this->boundingRect().toRect(),
                            *_tile);
    else
    {
        QString string;
        if (_tileSource.isNull())
            string = " No tile source defined";
        else
            string = " Loading...";
        painter->drawText(this->boundingRect(),
                          string,
                          QTextOption(Qt::AlignCenter));
    }
}

quint16 MapTileGraphicsObject::tileSize() const
{
    return _tileSize;
}

void MapTileGraphicsObject::setTileSize(quint16 tileSize)
{
    if (tileSize == _tileSize)
        return;
    _tileSize = tileSize;
}

void MapTileGraphicsObject::setTile(quint32 x, quint32 y, quint8 z, bool force)
{
    //Don't re-request the same tile we're already displaying unless force=true or _initialized=false
    if (_tileX == x && _tileY == y && _tileZoom == z && !force && _initialized)
        return;

    //Get rid of the old tile
    if (_tile != 0)
    {
        delete _tile;
        _tile = 0;
    }

    //Store information for the tile we're requesting
    _tileX = x;
    _tileY = y;
    _tileZoom = z;
    _initialized = true;

    //If we have a null tile source, there's not much more to do!
    if (_tileSource.isNull())
        return;

    //If our tile source is good, connect to the signal we'll need to get the result after requesting
    connect(_tileSource.data(),
            SIGNAL(tileRetrieved(quint32,quint32,quint8)),
            this,
            SLOT(handleTileRetrieved(quint32,quint32,quint8)));

    //Make sure we know that we're requesting a tile
    _havePendingRequest = true;

    //Request the tile from tileSource, which will emit tileRetrieved when finished
    //qDebug() << this << "requests" << x << y << z;
    _tileSource->requestTile(x,y,z);
}

QSharedPointer<MapTileSource> MapTileGraphicsObject::tileSource() const
{
    return _tileSource;
}

void MapTileGraphicsObject::setTileSource(QSharedPointer<MapTileSource> nSource)
{
    //Disconnect from the old source, if applicable
    if (!_tileSource.isNull())
    {
        QObject::disconnect(_tileSource.data(),
                            SIGNAL(tileRetrieved(quint32,quint32,quint8)),
                            this,
                            SLOT(handleTileRetrieved(quint32,quint32,quint8)));
        QObject::disconnect(_tileSource.data(),
                            SIGNAL(allTilesInvalidated()),
                            this,
                            SLOT(handleTileInvalidation()));
    }

    //Set the new source
    QSharedPointer<MapTileSource> oldSource = _tileSource;
    _tileSource = nSource;

    //Connect signals if approprite
    if (!_tileSource.isNull())
    {
        connect(_tileSource.data(),
                SIGNAL(allTilesInvalidated()),
                this,
                SLOT(handleTileInvalidation()));
        //We connect/disconnect the "tileRetrieved" signal as needed and don't do it here!
    }

    //Force a refresh from the new source
    this->handleTileInvalidation();
}

//private slot
void MapTileGraphicsObject::handleTileRetrieved(quint32 x, quint32 y, quint8 z)
{
    //If we don't care about retrieved tiles (i.e., we haven't requested a tile), return
    //This shouldn't actually happen as the signal/slot should get disconnected
    if (!_havePendingRequest)
        return;

    //If this isn't the tile we're looking for, return
    else if (_tileX != x || _tileY != y || _tileZoom != z)
        return;

    //Now we know that our tile has been retrieved by the MapTileSource. We just need to get it.
    _havePendingRequest = false;

    //Make sure some mischevious person hasn't set our MapTileSource to null while we weren't looking...
    if (_tileSource.isNull())
        return;

    QImage * image = _tileSource->getFinishedTile(x,y,z);

    //Make sure someone didn't snake us to grabbing our tile
    if (image == 0)
    {
        qWarning() << "Failed to get tile" << x << y << z << "from MapTileSource";
        return;
    }

    //Convert the QImage to a QPixmap
    //We have to do this here since we can't use QPixmaps in non-GUI threads (i.e., MapTileSource)
    QPixmap * tile = new QPixmap();
    *tile = QPixmap::fromImage(*image);

    //Delete the QImage
    delete image;
    image = 0;

    //Make sure that the old tile has been disposed of. If it hasn't, do it
    //In reality, it should have been, so display a warning
    if (_tile != 0)
    {
        qWarning() << "Tile should be null, but isn't";
        delete _tile;
        _tile = 0;
    }

    //Set the new tile and force a redraw
    _tile = tile;
    this->update();

    //Disconnect our signal/slot connection with MapTileSource until we need to do another request
    //It remains to be seen if it's better to continually connect/reconnect or just to filter events
    QObject::disconnect(_tileSource.data(),
                        SIGNAL(tileRetrieved(quint32,quint32,quint8)),
                        this,
                        SLOT(handleTileRetrieved(quint32,quint32,quint8)));
}

//private slot
void MapTileGraphicsObject::handleTileInvalidation()
{
    //If we haven't been initialized with a proper tile coordinate to fetch yet, don't force a refresh
    if (!_initialized)
        return;

    //Call setTile with force=true so that it forces a refresh
    this->setTile(_tileX,_tileY,_tileZoom,true);
}
