#include "PrivateQGraphicsView.h"

#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QtDebug>

PrivateQGraphicsView::PrivateQGraphicsView(QWidget *parent) :
    QGraphicsView(parent)
{
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

PrivateQGraphicsView::PrivateQGraphicsView(QGraphicsScene *scene, QWidget *parent) :
    QGraphicsView(scene,parent)
{
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

PrivateQGraphicsView::~PrivateQGraphicsView()
{
}

//protected
////virtual from QGraphicsView
void PrivateQGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->setAccepted(false);
    this->hadMouseDoubleClickEvent(event);
    if (!event->isAccepted())
        QGraphicsView::mouseDoubleClickEvent(event);
}

//protected
////virtual from QGraphicsView
void PrivateQGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(false);
    this->hadMouseMoveEvent(event);
    if (!event->isAccepted())
        QGraphicsView::mouseMoveEvent(event);
}

//protected
////virtual from QGraphicsView
void PrivateQGraphicsView::mousePressEvent(QMouseEvent *event)
{
    event->setAccepted(false);
    this->hadMousePressEvent(event);
    if (!event->isAccepted())
        QGraphicsView::mousePressEvent(event);
}

//protected
////virtual from QGraphicsView
void PrivateQGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    event->setAccepted(false);
    this->hadMouseReleaseEvent(event);
    if (!event->isAccepted())
        QGraphicsView::mouseReleaseEvent(event);
}

//protected
////virtual from QGraphicsView
void PrivateQGraphicsView::contextMenuEvent(QContextMenuEvent *event)
{
    event->setAccepted(false);
    this->hadContextMenuEvent(event);
    if (!event->isAccepted())
        QGraphicsView::contextMenuEvent(event);
}

//protected
//virtual from QGraphicsView
void PrivateQGraphicsView::wheelEvent(QWheelEvent *event)
{
    event->setAccepted(false);
    this->hadWheelEvent(event);
    if (!event->isAccepted())
        QGraphicsView::wheelEvent(event);
}
