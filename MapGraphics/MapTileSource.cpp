#include "MapTileSource.h"

#include <QStringBuilder>
#include <QMutexLocker>
#include <QtDebug>
#include <QStringList>
#include <QDataStream>

const QString MAPGRAPHICS_CACHE_FOLDER_NAME = ".MapGraphicsCache";
const quint32 DEFAULT_CACHE_DAYS = 7;
const quint64 MAX_DISK_CACHE_READ_ATTEMPTS = 100000;

MapTileSource::MapTileSource() :
    QObject(), _cacheExpirationsLoaded(false)
{
    this->setCacheMode(DiskAndMemCaching);

    //We connect this signal/slot pair to communicate across threads.
    connect(this,
            SIGNAL(tileRequested(quint32,quint32,quint8)),
            this,
            SLOT(startTileRequest(quint32,quint32,quint8)),
            Qt::QueuedConnection);

    /*
      When all our tiles have been invalidated, we clear our temp cache so any misinformed clients
      that don't notice will get a null tile instead of an old tile.
    */
    connect(this,
            SIGNAL(allTilesInvalidated()),
            this,
            SLOT(clearTempCache()));
}

MapTileSource::~MapTileSource()
{
    this->saveCacheExpirationsToDisk();
}

void MapTileSource::requestTile(quint32 x, quint32 y, quint8 z)
{
    /*We emit a signal to communicate across threads. MapTileSource (usually) runs in its own
      thread, but this method will be called from a different thread (probably the GUI thread).
      It's easy to communicate across threads with queued signals/slots.
    */
    this->tileRequested(x,y,z);
}

QImage *MapTileSource::getFinishedTile(quint32 x, quint32 y, quint8 z)
{
    const QString cacheID = MapTileSource::createCacheID(x,y,z);
    QMutexLocker lock(&_tempCacheLock);
    if (!_tempCache.contains(cacheID))
    {
        qWarning() << "getFinishedTile() called, but the tile is not present";
        return 0;
    }
    return _tempCache.take(cacheID);
}

MapTileSource::CacheMode MapTileSource::cacheMode() const
{
    return _cacheMode;
}

void MapTileSource::setCacheMode(MapTileSource::CacheMode nMode)
{
    _cacheMode = nMode;
}

//private slot
void MapTileSource::startTileRequest(quint32 x, quint32 y, quint8 z)
{
    //Check caches for the tile first
    if (this->cacheMode() == DiskAndMemCaching)
    {
        const QString cacheID = MapTileSource::createCacheID(x,y,z);
        QImage * cached = this->fromMemCache(cacheID);
        if (!cached)
            cached = this->fromDiskCache(cacheID);

        //If we got an image from one of the caches, prepare it for the client and return
        if (cached)
        {
            this->prepareRetrievedTile(x,y,z,cached);
            return;
        }
    }

    //If we get here, the tile was not cached and we must try to retrieve it
    this->fetchTile(x,y,z);
}

//private slot
void MapTileSource::clearTempCache()
{
    _tempCache.clear();
}

//protected static
QString MapTileSource::createCacheID(quint32 x, quint32 y, quint8 z)
{
    //We use % because it's more efficient to concatenate with QStringBuilder
    QString toRet = QString::number(x) % "," % QString::number(y) % "," % QString::number(z);
    return toRet;
}

//protected static
bool MapTileSource::cacheID2xyz(const QString & string, quint32 *x, quint32 *y, quint32 *z)
{
    QStringList list = string.split(',');
    if (list.size() != 3)
    {
        qWarning() << "Bad cacheID" << string << "cannot convert";
        return false;
    }

    bool ok = true;
    *x = list.at(0).toUInt(&ok);
    if (!ok)
        return false;
    *y = list.at(1).toUInt(&ok);
    if (!ok)
        return false;
    *z = list.at(2).toUInt(&ok);
    return ok;
}

QImage *MapTileSource::fromMemCache(const QString &cacheID)
{
    QImage * toRet = 0;

    if (_memoryCache.contains(cacheID))
    {
        //Figure out when the tile we're loading from cache was supposed to expire
        QDateTime expireTime = this->getTileExpirationTime(cacheID);

        //If the cached tile is older than we would like, throw it out
        if (QDateTime::currentDateTimeUtc().secsTo(expireTime) <= 0)
        {
            _memoryCache.remove(cacheID);
        }
        //Otherwise, make a copy of the cached tile and return it to the caller
        else
        {
            toRet = new QImage(*_memoryCache.object(cacheID));
        }
    }

    return toRet;
}

void MapTileSource::toMemCache(const QString &cacheID, QImage *toCache, const QDateTime &expireTime)
{
    if (toCache == 0)
        return;

    if (_memoryCache.contains(cacheID))
        return;

    //Note when the tile will expire
    this->setTileExpirationTime(cacheID, expireTime);

    //Make a copy of the QImage
    QImage * copy = new QImage(*toCache);
    _memoryCache.insert(cacheID,copy);
}

QImage *MapTileSource::fromDiskCache(const QString &cacheID)
{
    //Figure out x,y,z based on the cacheID
    quint32 x,y,z;
    if (!MapTileSource::cacheID2xyz(cacheID,&x,&y,&z))
        return 0;

    //See if we've got it in the cache
    const QString path = this->getDiskCacheFile(x,y,z);
    QFile fp(path);
    if (!fp.exists())
        return 0;

    //Figure out when the tile we're loading from cache was supposed to expire
    QDateTime expireTime = this->getTileExpirationTime(cacheID);

    //If the cached tile is older than we would like, throw it out
    if (QDateTime::currentDateTimeUtc().secsTo(expireTime) <= 0)
    {
        if (!QFile::remove(path))
            qWarning() << "Failed to remove old cache file" << path;
        return 0;
    }

    if (!fp.open(QFile::ReadOnly))
    {
        qWarning() << "Failed to open" << QFileInfo(fp.fileName()).baseName() << "from cache";
        return 0;
    }

    QByteArray data;
    quint64 counter = 0;
    while (data.length() < fp.size())
    {
        data += fp.read(20480);
        if (++counter >= MAX_DISK_CACHE_READ_ATTEMPTS)
        {
            qWarning() << "Reading cache file" << fp.fileName() << "took too long. Aborting.";
            return 0;
        }
    }

    QImage * image = new QImage();
    if (!image->loadFromData(data))
    {
        delete image;
        return 0;
    }

    return image;
}

void MapTileSource::toDiskCache(const QString &cacheID, QImage *toCache, const QDateTime &expireTime)
{
    //Figure out x,y,z based on the cacheID
    quint32 x,y,z;
    if (!MapTileSource::cacheID2xyz(cacheID,&x,&y,&z))
        return;

    //Find out where we'll be caching
    const QString filePath = this->getDiskCacheFile(x,y,z);

    //If we've already cached something, do not cache it again
    QFile fp(filePath);
    if (fp.exists())
        return;

    //Note when the tile will expire
    this->setTileExpirationTime(cacheID, expireTime);

    //Auto-detect file format
    const char * format = 0;

    //No compression for lossy file types!
    const int quality = 100;

    //Try to write the data
    if (!toCache->save(filePath,format,quality))
        qWarning() << "Failed to put" << this->name() << x << y << z << "into disk cache";
}

void MapTileSource::prepareRetrievedTile(quint32 x, quint32 y, quint8 z, QImage *image)
{
    //Do tile sanity check here optionally
    if (image == 0)
        return;

    //Put it into the "temporary retrieval cache" so the user can grab it
    QMutexLocker lock(&_tempCacheLock);
    _tempCache.insert(MapTileSource::createCacheID(x,y,z),
                      image);
    /*
      We must explicitly unlock the mutex before emitting tileRetrieved in case
      we're running in the GUI thread (since the signal can trigger
      slots immediately in that case).
    */
    lock.unlock();

    //Emit signal so user knows to call getFinishedTile()
    this->tileRetrieved(x,y,z);
}

void MapTileSource::prepareNewlyReceivedTile(quint32 x, quint32 y, quint8 z, QImage *image, QDateTime expireTime)
{
    //Insert into caches when applicable
    const QString cacheID = MapTileSource::createCacheID(x,y,z);
    if (this->cacheMode() == DiskAndMemCaching)
    {
        this->toMemCache(cacheID, image, expireTime);
        this->toDiskCache(cacheID, image, expireTime);
    }

    //Put the tile in a client-accessible place and notify them
    this->prepareRetrievedTile(x, y, z, image);
}

//protected
QDateTime MapTileSource::getTileExpirationTime(const QString &cacheID)
{
    //Make sure we've got our expiration database loaded
    this->loadCacheExpirationsFromDisk();

    QDateTime expireTime;
    if (_cacheExpirations.contains(cacheID))
        expireTime = _cacheExpirations.value(cacheID);
    else
    {
        qWarning() << "Tile" << cacheID << "has unknown expire time. Resetting to default of" << DEFAULT_CACHE_DAYS << "days.";
        expireTime = QDateTime::currentDateTimeUtc().addDays(DEFAULT_CACHE_DAYS);
        _cacheExpirations.insert(cacheID,expireTime);
    }

    return expireTime;
}

//protected
void MapTileSource::setTileExpirationTime(const QString &cacheID, QDateTime expireTime)
{
    //Make sure we've got our expiration database loaded
    this->loadCacheExpirationsFromDisk();

    //If they told us when the tile expires, store that expiration. Otherwise, use the default.
    if (expireTime.isNull())
    {
        expireTime = QDateTime::currentDateTimeUtc().addDays(DEFAULT_CACHE_DAYS);
    }

    _cacheExpirations.insert(cacheID, expireTime);
}

//private
QDir MapTileSource::getDiskCacheDirectory(quint32 x, quint32 y, quint8 z) const
{
    Q_UNUSED(y)
    QString pathString = QDir::homePath() % "/" % MAPGRAPHICS_CACHE_FOLDER_NAME % "/" % this->name() % "/" % QString::number(z) % "/" % QString::number(x);
    QDir toRet = QDir(pathString);
    if (!toRet.exists())
    {
        if (!toRet.mkpath(toRet.absolutePath()))
            qWarning() << "Failed to create cache directory" << toRet.absolutePath();
    }
    return toRet;
}

//private
QString MapTileSource::getDiskCacheFile(quint32 x, quint32 y, quint8 z) const
{
    QString toRet = this->getDiskCacheDirectory(x,y,z).absolutePath() % "/" % QString::number(y) % "." % this->tileFileExtension();
    return toRet;
}

//private
void MapTileSource::loadCacheExpirationsFromDisk()
{
    if (_cacheExpirationsLoaded)
        return;

    //If we try to do this and succeed or even fail, don't try again
    _cacheExpirationsLoaded = true;

    QDir dir = this->getDiskCacheDirectory(0,0,0);
    QString path = dir.absolutePath() % "/" % "cacheExpirations.db";
    _cacheExpirationsFile = path;

    QFile fp(path);
    if (!fp.exists())
        return;

    if (!fp.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open cache expiration file for reading:" << fp.errorString();
        return;
    }

    QDataStream stream(&fp);
    stream >> _cacheExpirations;
}

void MapTileSource::saveCacheExpirationsToDisk()
{
    if (!_cacheExpirationsLoaded || _cacheExpirationsFile.isEmpty())
        return;

    QFile fp(_cacheExpirationsFile);

    if (!fp.open(QIODevice::Truncate | QIODevice::WriteOnly))
    {
        qWarning() << "Failed to open cache expiration file for writing:" << fp.errorString();
        return;
    }

    QDataStream stream(&fp);
    stream << _cacheExpirations;
    qDebug() << "Cache expirations saved to" << _cacheExpirationsFile;
}
