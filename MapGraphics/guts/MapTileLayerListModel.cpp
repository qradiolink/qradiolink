#include "MapTileLayerListModel.h"

#include <QModelIndex>
#include <QtDebug>

MapTileLayerListModel::MapTileLayerListModel(QWeakPointer<CompositeTileSource> composite,QObject *parent) :
    QAbstractListModel(parent), _composite(composite)
{
    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return;
    CompositeTileSource * raw = strong.data();

    //When the model (CompositeTileSource) changes, reload data
    connect(raw,
            SIGNAL(sourcesChanged()),
            this,
            SLOT(handleCompositeSourcesChanged()));
    connect(raw,
            SIGNAL(sourceAdded(int)),
            this,
            SLOT(handleCompositeSourcesAdded(int)));
    connect(raw,
            SIGNAL(sourceRemoved(int)),
            this,
            SLOT(handleCompositeSourcesRemoved(int)));
}

int MapTileLayerListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return 0;

    return strong->numSources();
}

QVariant MapTileLayerListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant("Invalid index");

    if (index.row() >= this->rowCount())
        return QVariant("Index out of bounds");

    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return QVariant("Null composite");

    if (role == Qt::DisplayRole)
    {
        int i = index.row();
        QSharedPointer<MapTileSource> tileSource = strong->getSource(i);
        if (tileSource.isNull())
            return QVariant("Error: Null source");
        else
            return tileSource->name();
    }
    else
        return QVariant();
}

Qt::ItemFlags MapTileLayerListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return (Qt::ItemIsSelectable| Qt::ItemIsEnabled);
}

void MapTileLayerListModel::handleCompositeSourcesChanged()
{
    QModelIndex topleft = this->index(0);
    QModelIndex bottomright = this->index(this->rowCount());
    this->dataChanged(topleft,bottomright);
}

void MapTileLayerListModel::handleCompositeSourcesAdded(int index)
{
    this->beginInsertRows(QModelIndex(),
                          index,
                          index);
    this->endInsertRows();
}

void MapTileLayerListModel::handleCompositeSourcesRemoved(int index)
{
    this->beginRemoveRows(QModelIndex(),
                          index,
                          index);
    this->endRemoveRows();
}
