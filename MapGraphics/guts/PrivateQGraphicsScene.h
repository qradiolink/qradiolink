#ifndef PRIVATEQGRAPHICSSCENE_H
#define PRIVATEQGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <QHash>
#include <QPointer>
#include <QWeakPointer>

#include "MapGraphicsScene.h"
#include "MapGraphicsObject.h"
#include "PrivateQGraphicsObject.h"
#include "guts/PrivateQGraphicsInfoSource.h"

class PrivateQGraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit PrivateQGraphicsScene(MapGraphicsScene * mgScene,
                                   PrivateQGraphicsInfoSource * infoSource,
                                   QObject *parent = 0);
    
signals:
    
public slots:

private slots:
    void handleMGObjectAdded(MapGraphicsObject *);
    void handleMGObjectRemoved(MapGraphicsObject *);
    void handleZoomLevelChanged();

    void handleSelectionChanged();

private:
    void setMapGraphicsScene(MapGraphicsScene * mgScene);

    QPointer<MapGraphicsScene> _mgScene;
    PrivateQGraphicsInfoSource * _infoSource;

    QHash<MapGraphicsObject *,PrivateQGraphicsObject *> _mgToqg;

    QList<QGraphicsItem *> _oldSelections;
    
};

#endif // PRIVATEQGRAPHICSSCENE_H
