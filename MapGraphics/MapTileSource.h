#ifndef MAPTILESOURCE_H
#define MAPTILESOURCE_H

#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QImage>
#include <QCache>
#include <QMutex>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHash>

#include "MapGraphics_global.h"

class MAPGRAPHICSSHARED_EXPORT MapTileSource : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Enum used to describe how a MapTileSource should cache tiles, if at all. NoCaching does not
     * cache tiles at all. DiskAndMemCaching caches tiles on disk and in RAM
     *
     */
    enum CacheMode
    {
        NoCaching,
        DiskAndMemCaching
    };

public:
    explicit MapTileSource();
    virtual ~MapTileSource();

    /**
     * @brief Causes the MapTileSource to request the tile (x,y) at zoom level z.
     * A tileRetrieved signal will be emitted when the tile is available, at which point it can be
     * retrieved using getFinishedTile()
     *
     * @param x
     * @param y
     * @param z
     */
    void requestTile(quint32 x, quint32 y, quint8 z);

    /**
     * @brief Retrieves a pointer to a retrieved image tile. You must call requestTile and wait for the
     * tileRetrieved signal before calling this method. Returns a QImage pointer on success, null on failure.
     * The caller takes ownership of the QImage pointer - i.e., the caller is responsible for deleting it.
     *
     * @param x
     * @param y
     * @param z
     * @return QImage
     */
    QImage * getFinishedTile(quint32 x, quint32 y, quint8 z);

    MapTileSource::CacheMode cacheMode() const;

    void setCacheMode(MapTileSource::CacheMode);

    /**
     * @brief Converst from geo (lat,lon) coordinates into QGraphicsScene coordinates. A MapTileSource
     * implementation has to implement this method.
     *
     * @param ll the lat,lon to convert
     * @param zoomLevel the zoom-level used to convert
     * @return QPointF a point in QGraphicsScene coordinates
     */
    virtual QPointF ll2qgs(const QPointF& ll, quint8 zoomLevel) const=0;

    /**
     * @brief Converts from QGraphicsScene coordinates into geo (lat,lon) coordinates. A MapTileSource
     * implementation has to implement this method.
     *
     * @param qgs a point in pixel (QGraphicsScene) coordinates
     * @param zoomLevel the zoom-level used to convert
     * @return QPointF a point in geo (lat,lon) coordinates
     */
    virtual QPointF qgs2ll(const QPointF& qgs, quint8 zoomLevel) const=0;

    /**
     * @brief Pure-virtual method that returns the number of tiles on a given zoom level.
     *
     * @param zoomLevel
     * @return quint64
     */
    virtual quint64 tilesOnZoomLevel(quint8 zoomLevel) const=0;

    /**
     * @brief Returns the size of the map tiles in pixels. Tiles are assumed to be squares with the length
     * and width returned by this method. A MapTileSource implementation must implement this method.
     *
     * @return quint16 the length/width of the map tiles
     */
    virtual quint16 tileSize() const=0;

    /**
     * @brief Returns the minimum zoom level (the most zoomed-out) available at a given lat/lon. A
     * MapTileSource implementation has to implement this method.
     *
     * @param ll
     * @return quint8
     */
    virtual quint8 minZoomLevel(QPointF ll = QPointF())=0;

    /**
     * @brief Returns the maximum zoom level (the most zoomed-in) available at a given lat/lon. A
     * MapTileSource implementation has to implement this method.
     *
     * @param ll
     * @return quint8
     */
    virtual quint8 maxZoomLevel(QPointF ll = QPointF())=0;

    /**
     * @brief Returns the name of this MapTileSource. A MapTileSource implementation has to implement this
     * method.
     *
     * @return QString
     */
    virtual QString name() const=0;

    /**
     * @brief Returns the file extension (jpg,png,etc) of the image tilees returne by the source. Don't
     * include the . in the extension. e.g., return jpg instead of .jpg. A MapTileSource implementation has to
     * implement this method.
     *
     */
    virtual QString tileFileExtension() const=0;
    
signals:
    /**
     * @brief Signal emitted when a tile that was requested with requestTile() has been retrieved and can
     * be gotten at using getFinishedTile()
     *
     * @param x
     * @param y
     * @param z
     */
    void tileRetrieved(quint32 x, quint32 y, quint8 z);

    /**
     * @brief Signal emitted when a tile is requested using requestTile().
     *
     * @param x
     * @param y
     * @param z
     */
    void tileRequested(quint32 x, quint32 y, quint8 z);

    /*!
     \brief Emitted when vital parameters of the tile source have changed and anyone displaying the tiles should
      refresh.

    */
    void allTilesInvalidated();
    
public slots:

private slots:
    void startTileRequest(quint32 x, quint32 y, quint8 z);
    void clearTempCache();

protected:
    /**
     * @brief This static method takes the x,y,z of a tile and creates a unique string that is used
     * as a key in the caches to keep track of the tile.
     *
     * @param x x-coordinate of the tile
     * @param y y-coordinate of the tile
     * @param z zoom-level of the tile
     * @return QString unique cacheID
     */
    static QString createCacheID(quint32 x, quint32 y, quint8 z);

    /**
     * @brief This static convenience method takes a cacheID and places the numberical x,y,z values that
     * generated the cacheID into the variables pointed to by x,y,z. Returns true on success, false on failure.
     *
     * @param string the cacheID you are converting
     * @param x where you want the x value
     * @param y where you want the y value
     * @param z where you want the z value
     * @return bool
     */
    static bool cacheID2xyz(const QString & string, quint32 * x, quint32 * y, quint32 * z);

    /**
     * @brief Given a cacheID, retrieve the tile with that cacheID from memcache. Returns a pointer
     * to a QImage on success, null on failure. Caller takes responsibility for deleting the returned
     * QImage
     *
     * @param cacheID cacheID of the tile you want to get from cache
     * @return QImage
     */
    QImage * fromMemCache(const QString& cacheID);

    /**
     * @brief Given a cacheID and a pointer to a QImage, inserts the QImage pointed to by the pointer into
     * the memory cache using cacheID as the key
     *
     * @param cacheID
     * @param toCache
     */
    void toMemCache(const QString& cacheID, QImage * toCache, const QDateTime &expireTime = QDateTime());

    /**
     * @brief Given a cacheID, retrieve the tile with that cacheID from the disk cache. Returns a
     * pointer to a QImage on success, null on failure. Caller takes responsibility for deleting the
     * returned QImage
     *
     * @param cacheID cacheID of the tile you want to get from cache
     * @return QImage
     */
    QImage * fromDiskCache(const QString& cacheID);

    /**
     * @brief Given a cacheID and a pointer to a QImage, inserts the QImage pointed to by the pointer into
     * the disk cache using cacheID as the key.
     * Optionally, takes a QDateTime object that specifies the time that the QImage should be kept cached 
     * until. Defaults to 7 days.
     *
     * @param cacheID
     * @param toCache
     * @param cacheUntil
     */
    void toDiskCache(const QString& cacheID, QImage * toCache, const QDateTime &expireTime = QDateTime());

    /**
     * @brief Fetches (from MapQuest or OSM or whatever) or generates the tile if it isn't cached.
     * This is where the rubber hits the road, so to speak, for a MapTileSource. When successful, this method
     * should just call prepareRetrievedTile. On failure, do nothing.
     *
     * @param x x-coordinate of the tile
     * @param y y-coordinate of the tile
     * @param z zoom-level of the tile
     * @return bool
     */
    virtual void fetchTile(quint32 x,
                           quint32 y,
                           quint8 z)=0;

    //Call only for tiles which were newly-generated or newly-acquired from the network (i.e., not cached)
    void prepareNewlyReceivedTile(quint32 x, quint32 y, quint8 z, QImage * image, QDateTime expireTime = QDateTime());

    /**
     * @brief Returns the time when the tile is supposed to expire from any caches.
     * This should only be called on tiles which are actually cached!
     * @param cacheID The cacheID of the tile
     * @return QDateTime of the tile's expiration (time after which it should be re-requested or regenerated)
     */
    QDateTime getTileExpirationTime(const QString& cacheID);

    /**
     * @brief Sets the time when the tile is supposed to expire from any caches
     * @param cacheID of the tile
     * @param QDateTime of the tile's expiration (time after which it should be re-requested or regenerated)
     */
    void setTileExpirationTime(const QString& cacheID, QDateTime expireTime);

private:
    /**
     * @brief prepareRetrievedTile prepares a generated/retrieve tile for retrieval by the client
     * and notifies the client that the tile is ready.
     */
    void prepareRetrievedTile(quint32 x, quint32 y, quint8 z, QImage * image);

    /**
     * @brief Given the x,y, and z of a tile, returns the directory where it should be cached on disk
     *
     * @param x
     * @param y
     * @param z
     * @return QDir
     */
    QDir getDiskCacheDirectory(quint32 x, quint32 y, quint8 z) const;

    /**
     * @brief Given the x,y, and z of a tile, returns the full path to the file where it should be
     * cached on disk
     *
     * @param x
     * @param y
     * @param z
     * @return QString
     */
    QString getDiskCacheFile(quint32 x, quint32 y, quint8 z) const;

    /*!
     \brief Loads cache expiration times from disk if necessary
    */
    void loadCacheExpirationsFromDisk();

    void saveCacheExpirationsToDisk();

    bool _cacheExpirationsLoaded;

    /*
      loadCacheExpirationsFromDisk() stores the place where it loaded from here.
      When we're destructing and need to serialize out we can use this instead of
      foolishly trying to call a pure-virtual method (name()) from the destructor
    */
    QString _cacheExpirationsFile;

    MapTileSource::CacheMode _cacheMode;

    //Temporary cache for QImage tiles waiting for the client to take them
    QCache<QString, QImage> _tempCache;
    QMutex _tempCacheLock;

    //The "real" cache, where tiles are saved in memory so we don't download them again
    QCache<QString, QImage> _memoryCache;

    QHash<QString, QDateTime> _cacheExpirations;
    
};

#endif // MAPTILESOURCE_H
