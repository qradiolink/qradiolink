#include "CompositeTileSource.h"

#include <QtDebug>
#include <QPainter>
#include <QMutexLocker>
#include <QThread>
#include <QPointer>
#include <QTimer>

CompositeTileSource::CompositeTileSource() :
    MapTileSource()
{
    _globalMutex = new QMutex(QMutex::Recursive);
    this->setCacheMode(MapTileSource::NoCaching);
}

CompositeTileSource::~CompositeTileSource()
{
    _globalMutex->lock();

    qDebug() << this << "destructing";
    //Clean up all data related to pending tiles
    this->clearPendingTiles();

    //Clear the sources
    //We first find all the sources' threads and put them in a list
    QList<QPointer<QThread> > tileSourceThreads;
    foreach(QSharedPointer<MapTileSource> source, _childSources)
        tileSourceThreads.append(QPointer<QThread>(source->thread()));

    //Then we clear the sources
    _childSources.clear();

    int numThreads = tileSourceThreads.size();
    //Then we wait for all of those threads to shut down
    for (int i = 0; i < numThreads; i++)
    {
        QPointer<QThread> thread = tileSourceThreads[i];
        if (!thread.isNull() && thread != this->thread())
            thread->wait(10000);
    }

    delete this->_globalMutex;
}

QPointF CompositeTileSource::ll2qgs(const QPointF &ll, quint8 zoomLevel) const
{
    QMutexLocker lock(_globalMutex);
    if (_childSources.isEmpty())
    {
        qWarning() << "Composite tile source is empty --- results undefined";
        return QPointF(0,0);
    }

    //Assume they're all the same. Nothing to do otherwise!
    return _childSources.at(0)->ll2qgs(ll,zoomLevel);
}

QPointF CompositeTileSource::qgs2ll(const QPointF &qgs, quint8 zoomLevel) const
{
    QMutexLocker lock(_globalMutex);
    if (_childSources.isEmpty())
    {
        qWarning() << "Composite tile source is empty --- results undefined";
        return QPointF(0,0);
    }

    //Assume they're all the same. Nothing to do otherwise!
    return _childSources.at(0)->qgs2ll(qgs,zoomLevel);
}

quint64 CompositeTileSource::tilesOnZoomLevel(quint8 zoomLevel) const
{
    QMutexLocker lock(_globalMutex);
    if (_childSources.isEmpty())
        return 1;
    else
        return _childSources.at(0)->tilesOnZoomLevel(zoomLevel);
}

quint16 CompositeTileSource::tileSize() const
{
    QMutexLocker lock(_globalMutex);
    if (_childSources.isEmpty())
        return 256;
    else
        return _childSources.at(0)->tileSize();
}

quint8 CompositeTileSource::minZoomLevel(QPointF ll)
{
    QMutexLocker lock(_globalMutex);
    //Return the highest minimum
    quint8 highest = 0;

    foreach(QSharedPointer<MapTileSource> source, _childSources)
    {
        quint8 current = source->minZoomLevel(ll);
        if (current > highest)
            highest = current;
    }
    return highest;
}

quint8 CompositeTileSource::maxZoomLevel(QPointF ll)
{
    QMutexLocker lock(_globalMutex);
    //Return the lowest maximum
    quint8 lowest = 50;

    foreach(QSharedPointer<MapTileSource> source, _childSources)
    {
        quint8 current = source->maxZoomLevel(ll);
        if (current < lowest)
            lowest = current;
    }
    return lowest;
}

QString CompositeTileSource::name() const
{
    return "Composite Tile Source";
}

QString CompositeTileSource::tileFileExtension() const
{
    return ".jpg";
}

void CompositeTileSource::addSourceTop(QSharedPointer<MapTileSource> source, qreal opacity)
{
    QMutexLocker lock(_globalMutex);
    if (source.isNull())
        return;

    //Put the child in its own thread
    this->doChildThreading(source);

    _childSources.insert(0, source);
    _childOpacities.insert(0,opacity);
    _childEnabledFlags.insert(0,true);

    connect(source.data(),
            SIGNAL(tileRetrieved(quint32,quint32,quint8)),
            this,
            SLOT(handleTileRetrieved(quint32,quint32,quint8)));

    this->sourceAdded(0);
    this->sourcesChanged();
    this->allTilesInvalidated();
}

void CompositeTileSource::addSourceBottom(QSharedPointer<MapTileSource> source, qreal opacity)
{
    QMutexLocker lock(_globalMutex);
    if (source.isNull())
        return;

    //Put the child in its own thread
    this->doChildThreading(source);

    _childSources.append(source);
    _childOpacities.append(opacity);
    _childEnabledFlags.append(true);

    connect(source.data(),
            SIGNAL(tileRetrieved(quint32,quint32,quint8)),
            this,
            SLOT(handleTileRetrieved(quint32,quint32,quint8)));

    this->sourceAdded(_childSources.size()-1);
    this->sourcesChanged();
    this->allTilesInvalidated();
}

void CompositeTileSource::moveSource(int from, int to)
{
    if (from < 0 || to < 0)
        return;

    QMutexLocker lock(_globalMutex);

    int size = this->numSources();
    if (from >= size || to >= size)
        return;

    _childSources.move(from,to);
    _childOpacities.move(from,to);
    _childEnabledFlags.move(from,to);

    this->sourcesReordered();
    this->sourcesChanged();
    this->allTilesInvalidated();
}

void CompositeTileSource::removeSource(int index)
{
    QMutexLocker lock(_globalMutex);
    if (index < 0 || index >= _childSources.size())
        return;

    _childSources.removeAt(index);
    _childOpacities.removeAt(index);
    _childEnabledFlags.removeAt(index);
    this->clearPendingTiles();

    this->sourceRemoved(index);
    this->sourcesChanged();
    this->allTilesInvalidated();
}

int CompositeTileSource::numSources() const
{
    QMutexLocker lock(_globalMutex);
    return _childSources.size();
}

QSharedPointer<MapTileSource> CompositeTileSource::getSource(int index) const
{
    QMutexLocker lock(_globalMutex);
    if (index < 0 || index >= _childSources.size())
        return QSharedPointer<MapTileSource>();

    return _childSources[index];
}

qreal CompositeTileSource::getOpacity(int index) const
{
    QMutexLocker lock(_globalMutex);
    if (index < 0 || index >= _childSources.size())
        return 0.0;
    return _childOpacities[index];
}

void CompositeTileSource::setOpacity(int index, qreal opacity)
{
    opacity = qMin<qreal>(1.0,qMax<qreal>(0.0,opacity));

    QMutexLocker lock(_globalMutex);
    if (index < 0 || index >= _childSources.size())
        return;

    if (_childOpacities[index] == opacity)
        return;

    _childOpacities[index] = opacity;

    //emit signal to tell any models watching us that we've changed
    this->sourcesChanged();
    this->allTilesInvalidated();
}

bool CompositeTileSource::getEnabledFlag(int index) const
{
    QMutexLocker lock(_globalMutex);
    if (index < 0 || index >= _childSources.size())
        return 0.0;
    return _childEnabledFlags[index];
}

void CompositeTileSource::setEnabledFlag(int index, bool isEnabled)
{
    QMutexLocker lock(_globalMutex);
    if (index < 0 || index >= _childSources.size())
        return;

    if (_childEnabledFlags[index] == isEnabled)
        return;

    _childEnabledFlags[index] = isEnabled;

    this->sourcesChanged();
    this->allTilesInvalidated();
}

//protected
void CompositeTileSource::fetchTile(quint32 x, quint32 y, quint8 z)
{
    QMutexLocker lock(_globalMutex);
    //If we have no child sources, just print a message about that
    if (_childSources.isEmpty())
    {
        QImage * toRet = new QImage(this->tileSize(),
                                    this->tileSize(),
                                    QImage::Format_ARGB32_Premultiplied);
        QPainter painter(toRet);
        painter.fillRect(toRet->rect(),
                         Qt::white);
        painter.drawText(toRet->rect(),
                         QString("Composite Source Empty"),
                         QTextOption(Qt::AlignCenter));
        painter.end();
        this->prepareNewlyReceivedTile(x,y,z,toRet);
        return;
    }

    //Allocate space in memory to store the tiles as they come before we composite them.
    //If we already have a space allocated from a previous un-finished request, clear it and start over
    QString cacheID = MapTileSource::createCacheID(x,y,z);
    if (_pendingTiles.contains(cacheID))
    {
        QMap<quint32, QImage *> * tiles = _pendingTiles.value(cacheID);
        foreach(QImage * tile, *tiles)
            delete tile;
        tiles->clear();
    }
    //Otherwise, create a new space
    else
        _pendingTiles.insert(cacheID,new QMap<quint32,QImage *>());

    //Request tiles from all of our beautiful children
    for (int i = 0; i < _childSources.size(); i++)
    {
        QSharedPointer<MapTileSource> child = _childSources.at(i);
        child->requestTile(x,y,z);
    }
}

//private slot
void CompositeTileSource::handleTileRetrieved(quint32 x, quint32 y, quint8 z)
{
    QMutexLocker lock(_globalMutex);
    QObject * sender = QObject::sender();
    MapTileSource * tileSource = qobject_cast<MapTileSource *>(sender);

    //Make sure this slot was called from a signal off a MapTileSource
    if (!tileSource)
    {
        qWarning() << this << "failed MapTileSource cast";
        return;
    }

    //Make sure this is a notification from a MapTileSource that we care about
    int tileSourceIndex = -1;
    for (int i = 0; i < _childSources.size(); i++)
    {
        if (_childSources[i].data() != tileSource)
            continue;
        tileSourceIndex = i;
        break;
    }

    if (tileSourceIndex == -1)
    {
        qWarning() << this << "received tile from unknown source...";
        return;
    }


    //Make sure that this is a tile we're interested in
    const QString cacheID = MapTileSource::createCacheID(x,y,z);
    if (!_pendingTiles.contains(cacheID))
    {
        qWarning() << this << "received unknown tile" << x << y << z << "from" << tileSource;
        return;
    }

    //Make sure the tile is non-null
    QImage * tile = tileSource->getFinishedTile(x,y,z);
    if (!tile)
    {
        qWarning() << this << "received null tile" << x << y << z << "from" << tileSource;
        return;
    }

    //qDebug() << this << "Retrieved tile" << x << y << z << "from" << tileSource;

    /*
      Put the tile into our pendingTiles structure. If it was the last tile we wanted, build
      our finishied product and notify our client. If we've already received this tile because
      it was requested twice for some reason (e.g. crazy zooming in/out) then let's just go ahead
      and delete the new version and go about our day.
    */
    QMap<quint32, QImage *> * tiles = _pendingTiles.value(cacheID);
    if (tiles->contains(tileSourceIndex))
    {
        delete tile;
        return;
    }
    tiles->insert(tileSourceIndex,tile);

    //Still waiting for a tile or two?
    if (tiles->size() < _childSources.size())
        return;

    //Time to build the finished composite tile
    QImage * toRet = new QImage(this->tileSize(),
                                this->tileSize(),
                                QImage::Format_ARGB32_Premultiplied);
    QPainter painter(toRet);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setOpacity(1.0);
    for (int i = tiles->size()-1; i >= 0; i--)
    {
        QImage * childTile = tiles->value(i);
        qreal opacity = _childOpacities[i];

        //If there are no other layers, we need to be opaque no matter what
        if (this->numSources() == 1)
            opacity = 1.0;

        if (_childEnabledFlags[i] == false)
            opacity = 0.0;
        painter.setOpacity(opacity);
        painter.drawImage(0,0,*childTile);
        delete childTile;
    }
    delete tiles;
    _pendingTiles.remove(cacheID);
    painter.end();

    this->prepareNewlyReceivedTile(x,y,z,toRet);
}

//private slot
void CompositeTileSource::clearPendingTiles()
{
    QList<QMap<quint32, QImage *> * > pendingTiles = _pendingTiles.values();
    for (int i = 0; i < pendingTiles.size(); i++)
    {
        QMap<quint32, QImage *> * tiles = pendingTiles.at(i);
        foreach(QImage * tile, tiles->values())
            delete tile;
        delete tiles;
    }
    _pendingTiles.clear();
}

//private
void CompositeTileSource::doChildThreading(QSharedPointer<MapTileSource> source)
{
    if (source.isNull())
        return;

    //We create a new thread for each child source in our care.
    QThread * sourceThread = new QThread();
    sourceThread->start();
    source->moveToThread(sourceThread);

    //Set the thread up so that it will shutdown and be destroyed when the tilesource is dead
    connect(source.data(),
            SIGNAL(destroyed()),
            sourceThread,
            SLOT(quit()));
    connect(sourceThread,
            SIGNAL(finished()),
            sourceThread,
            SLOT(deleteLater()));
}
