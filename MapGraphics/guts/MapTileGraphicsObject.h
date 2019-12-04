#ifndef MAPTILEGRAPHICSOBJECT_H
#define MAPTILEGRAPHICSOBJECT_H

#include <QGraphicsObject>
#include <QPointer>

#include "MapTileSource.h"

class MapTileGraphicsObject : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit MapTileGraphicsObject(quint16 tileSize=256);
    ~MapTileGraphicsObject();

    virtual QRectF boundingRect() const;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    quint16 tileSize() const;
    void setTileSize(quint16 tileSize);

    void setTile(quint32 x, quint32 y, quint8 z, bool force = false);

    QSharedPointer<MapTileSource> tileSource() const;
    void setTileSource(QSharedPointer<MapTileSource>);


private slots:
    void handleTileRetrieved(quint32 x, quint32 y, quint8 z);
    void handleTileInvalidation();
    
signals:
    void tileRequested(quint32 x, quint32 y, quint8 z);
    
public slots:

private:
    quint16 _tileSize;
    QPixmap * _tile;
    quint32 _tileX;
    quint32 _tileY;
    quint8 _tileZoom;

    bool _initialized;

    bool _havePendingRequest;

    QSharedPointer<MapTileSource> _tileSource;
    
};

#endif // MAPTILEGRAPHICSOBJECT_H
