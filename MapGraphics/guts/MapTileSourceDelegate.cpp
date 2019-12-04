#include "MapTileSourceDelegate.h"

#include <QPainter>
#include <QtDebug>
#include <QPushButton>
#include <QStringBuilder>

MapTileSourceDelegate::MapTileSourceDelegate(QWeakPointer<CompositeTileSource> composite, QObject *parent) :
    QStyledItemDelegate(parent), _composite(composite)
{
}

//virtual from QStyledItemDelegate
void MapTileSourceDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return;
    QSharedPointer<MapTileSource> childSource = strong->getSource(index.row());
    if (childSource.isNull())
        return;

    painter->save();

    //Get the palette that we should use
    QPalette palette = option.palette;

    //Use a slightly smaller rect so that we can see our border better
    QRect rect = option.rect;
    rect.setWidth(rect.width()-2);

    //Based on palette and state, choose background color
    QBrush backgroundBrush = palette.base();
    QColor borderColor = palette.text().color();
    QColor textColor = palette.text().color();
    if (option.state & QStyle::State_Selected)
        backgroundBrush = palette.highlight();

    //Draw background
    painter->fillRect(rect,backgroundBrush);

    //Draw border
    painter->setPen(borderColor);
    painter->drawRect(rect);

    QFont nameFont = painter->font();
    QFont otherFont = painter->font();
    nameFont.setPointSize(nameFont.pointSize()+2);
    nameFont.setBold(true);

    //Draw the name of the source
    QRect textRect = rect;
    textRect.adjust(1,0,-1,0);
    QString nameString = childSource->name();
    painter->setPen(textColor);
    painter->setFont(nameFont);
    painter->drawText(textRect,nameString,QTextOption(Qt::AlignLeft | Qt::AlignTop));

    //Draw the opacity string
    QString opacityString = "Opacity: " % QString::number(strong->getOpacity(index.row()),'f',2);
    painter->setPen(textColor);
    painter->setFont(otherFont);
    painter->drawText(textRect,opacityString,QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

    //Draw the enabled/disabled string
    QString state = "Enabled";
    if (strong->getEnabledFlag(index.row()) == false)
        state = "Disabled";
    QString stateString = "Status: " % state;
    painter->setPen(textColor);
    painter->setFont(otherFont);
    painter->drawText(textRect,stateString,QTextOption(Qt::AlignLeft | Qt::AlignBottom));


    painter->restore();
}

QSize MapTileSourceDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    QSize toRet;
    toRet.setWidth(150);
    toRet.setHeight(50);

    return toRet;
}
