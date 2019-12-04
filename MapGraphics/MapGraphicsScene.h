#ifndef MAPGRAPHICSSCENE_H
#define MAPGRAPHICSSCENE_H

#include "MapGraphics_global.h"
#include "MapGraphicsObject.h"

#include <QObject>
#include <QList>
#include <QSet>

class MAPGRAPHICSSHARED_EXPORT MapGraphicsScene : public QObject
{
    Q_OBJECT
public:
    MapGraphicsScene(QObject * parent = 0);
    virtual ~MapGraphicsScene();

    /**
     * @brief Adds or re-parents the given MapGraphicsObject to this scene.
     *
     * @param item
     */
    void addObject(MapGraphicsObject * object);

    /**
     * @brief Returns a list of pointers to all MapGraphicsObject objects in the scene
     *
     * @return QList<MapGraphicsObject *>
     */
    QList<MapGraphicsObject *> objects() const;

    void removeObject(MapGraphicsObject * object);

signals:
    /**
     * @brief Fired when a MapGraphicsObject is added to the scene
     *
     * @param the object that was added
     */
    void objectAdded(MapGraphicsObject *);

    /**
     * @brief Fired when a MapGraphicsObject is removed from the scene.
     *
     * @param the object that was removed
     */
    void objectRemoved(MapGraphicsObject *);

private slots:
    void handleNewObjectGenerated(MapGraphicsObject * newObject);
    void handleObjectDestroyed(QObject * object);

private:
    QSet<MapGraphicsObject *> _objects;


};

#endif // MAPGRAPHICSSCENE_H
