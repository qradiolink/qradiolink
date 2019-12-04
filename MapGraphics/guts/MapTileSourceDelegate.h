#ifndef MAPTILESOURCEDELEGATE_H
#define MAPTILESOURCEDELEGATE_H

#include <QStyledItemDelegate>
#include <QWeakPointer>

#include "tileSources/CompositeTileSource.h"

class MapTileSourceDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MapTileSourceDelegate(QWeakPointer<CompositeTileSource> composite,QObject *parent = 0);

    //virtual from QStyledItemDelegate
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    //virtual from QStyledItemDelegate
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    
signals:
    
public slots:

private:
    QWeakPointer<CompositeTileSource> _composite;
    
};

#endif // MAPTILESOURCEDELEGATE_H
