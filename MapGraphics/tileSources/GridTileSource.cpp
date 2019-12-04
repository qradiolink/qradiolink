#include "GridTileSource.h"

#include <cmath>
#include <QPainter>
#include <QStringBuilder>
#include <QtDebug>

const qreal PI = 3.14159265358979323846;
const qreal deg2rad = PI / 180.0;
const qreal rad2deg = 180.0 / PI;

GridTileSource::GridTileSource() :
    MapTileSource()
{
    this->setCacheMode(MapTileSource::NoCaching);
}

GridTileSource::~GridTileSource()
{
    qDebug() << this << "destructing";
}

QPointF GridTileSource::ll2qgs(const QPointF &ll, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal x = (ll.x()+180.0) * (tilesOnOneEdge*tileSize)/360.0; // coord to pixel!
    qreal y = (1-(log(tan(PI/4.0+(ll.y()*deg2rad)/2)) /PI)) /2.0  * (tilesOnOneEdge*tileSize);

    return QPoint(int(x), int(y));
}

QPointF GridTileSource::qgs2ll(const QPointF &qgs, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal longitude = (qgs.x()*(360.0/(tilesOnOneEdge*tileSize)))-180.0;
    qreal latitude = rad2deg*(atan(sinh((1.0-qgs.y()*(2.0/(tilesOnOneEdge*tileSize)))*PI)));

    return QPointF(longitude, latitude);
}

quint64 GridTileSource::tilesOnZoomLevel(quint8 zoomLevel) const
{
    return pow(4.0,zoomLevel);
}

quint16 GridTileSource::tileSize() const
{
    return 256;
}

quint8 GridTileSource::minZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 0;
}

quint8 GridTileSource::maxZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 50;
}

QString GridTileSource::name() const
{
    return "Grid Tiles";
}

QString GridTileSource::tileFileExtension() const
{
    return ".png";
}

void GridTileSource::fetchTile(quint32 x, quint32 y, quint8 z)
{
    quint64 leftScenePixel = x*this->tileSize();
    quint64 topScenePixel = y*this->tileSize();
    quint64 rightScenePixel = leftScenePixel + this->tileSize();
    quint64 bottomScenePixel = topScenePixel + this->tileSize();

    QImage * toRet = new QImage(this->tileSize(),
                                this->tileSize(),
                                QImage::Format_ARGB32_Premultiplied);
    //It is important to fill with transparent first!
    toRet->fill(qRgba(0,0,0,0));

    QPainter painter(toRet);
    painter.setPen(Qt::black);
    //painter.fillRect(toRet->rect(),QColor(0,0,0,0));

    qreal everyNDegrees = 10.0;

    //Longitude
    for(qreal lon = -180.0; lon <= 180.0; lon += everyNDegrees)
    {
        QPointF geoPos(lon,0.0);
        QPointF qgsScenePos = this->ll2qgs(geoPos,z);

        if (qgsScenePos.x() < leftScenePixel || qgsScenePos.x() > rightScenePixel)
            continue;
        painter.drawLine(qgsScenePos.x() - leftScenePixel,
                         0,
                         qgsScenePos.x() - leftScenePixel,
                         this->tileSize());
    }

    //Latitude
    for (qreal lat = -90.0; lat < 90.0; lat += everyNDegrees)
    {
        QPointF geoPos(0.0,lat);
        QPointF qgsScenePos = this->ll2qgs(geoPos,z);

        if (qgsScenePos.y() < topScenePixel || qgsScenePos.y() > bottomScenePixel)
            continue;
        painter.drawLine(0,
                         qgsScenePos.y() - topScenePixel,
                         this->tileSize(),
                         qgsScenePos.y() - topScenePixel);
    }

    painter.end();

    this->prepareNewlyReceivedTile(x,y,z,toRet);
}
