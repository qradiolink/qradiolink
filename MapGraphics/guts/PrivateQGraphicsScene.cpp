#include "PrivateQGraphicsScene.h"

#include <QtDebug>
#include <QSet>

#include "MapGraphicsScene.h"

PrivateQGraphicsScene::PrivateQGraphicsScene(MapGraphicsScene * mgScene,
                                             PrivateQGraphicsInfoSource *infoSource,
                                             QObject *parent) :
    QGraphicsScene(parent), _infoSource(infoSource)
{
    this->setMapGraphicsScene(mgScene);

    connect(this,
            SIGNAL(selectionChanged()),
            this,
            SLOT(handleSelectionChanged()));
}

//private slot
void PrivateQGraphicsScene::handleMGObjectAdded(MapGraphicsObject * added)
{
    PrivateQGraphicsObject * qgObj = new PrivateQGraphicsObject(added,_infoSource);
    this->addItem(qgObj);

    //We need a mapping of MapGraphicsObject : QGraphicsObject, so put this in the map
    _mgToqg.insert(added,qgObj);
}

//private slot
void PrivateQGraphicsScene::handleMGObjectRemoved(MapGraphicsObject * removed)
{
    if (!_mgToqg.contains(removed))
    {
        qWarning() << "There is no QGraphicsObject in the scene for" << removed;
        return;
    }

    QGraphicsObject * qgObj = _mgToqg.take(removed);
    delete qgObj;

    /*
      It turns out that removing or "deleting later" the PrivateQGraphicsObjects here was causing crashes.
      Instead, the PrivateQGraphicsObject watches the MapGraphicsObject's destroyed signal to
      delete itself. QGraphicsScene is smart enough to remove deleted objects in this case.
    */
    /*
    if (!this->items().contains(qgObj))
    {
        qWarning() << this << "does not contain PrivateQGraphicsObject" << qgObj;
        return;
    }
    //qgObj->deleteLater();
    //this->removeItem(qgObj);
    */
}

void PrivateQGraphicsScene::handleZoomLevelChanged()
{
    foreach(PrivateQGraphicsObject * obj, _mgToqg.values())
        obj->handleZoomLevelChanged();
}

void PrivateQGraphicsScene::handleSelectionChanged()
{
    QList<QGraphicsItem *> selectedList = this->selectedItems();
    QSet<QGraphicsItem *> selected;
    foreach(QGraphicsItem * item, selectedList)
        selected.insert(item);

    QList<QGraphicsItem *> newSelections;
    foreach(PrivateQGraphicsObject * obj, _mgToqg.values())
    {
        QGraphicsItem * casted = (QGraphicsItem *) obj;

        bool doSelect = selected.contains(casted) && !_oldSelections.contains(obj);
        obj->setSelected(doSelect);

        if (doSelect)
            newSelections.append(obj);
    }

    _oldSelections = newSelections;

}

//private
void PrivateQGraphicsScene::setMapGraphicsScene(MapGraphicsScene *mgScene)
{
    _mgScene = mgScene;

    if (_mgScene.isNull())
    {
        qWarning() << this << "got a null MapGraphicsScene";
        return;
    }

    connect(_mgScene.data(),
            SIGNAL(objectAdded(MapGraphicsObject*)),
            this,
            SLOT(handleMGObjectAdded(MapGraphicsObject*)));
    connect(_mgScene.data(),
            SIGNAL(objectRemoved(MapGraphicsObject*)),
            this,
            SLOT(handleMGObjectRemoved(MapGraphicsObject*)));

}
