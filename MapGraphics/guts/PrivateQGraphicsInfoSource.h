#ifndef PRIVATEQGRAPHICSINFOSOURCE_H
#define PRIVATEQGRAPHICSINFOSOURCE_H

#include <QSharedPointer>

#include "MapTileSource.h"

/*!
 \brief This abstract class is inherited by MapGraphicsView as an implementation of
 the "dependency inversion" design pattern, or at least as well as I can remember it.

 Basically, PrivateQGraphicsObject and PrivateQGraphicsScene need information from
 MapGraphicsView, but MapGraphicsView is above those two classes.

 By making all references to MapGraphicsView in PrivateQGraphicsObject and PrivateQGraphicsScene
 go through this interface, we sort of eliminate an upwards-facing compile-time dependency.

*/
class PrivateQGraphicsInfoSource
{
public:
    virtual quint8 zoomLevel() const=0;

    virtual QSharedPointer<MapTileSource> tileSource() const=0;
};

#endif // PRIVATEQGRAPHICSINFOSOURCE_H
