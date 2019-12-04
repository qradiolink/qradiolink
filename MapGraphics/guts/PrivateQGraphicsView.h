#ifndef PRIVATEQGRAPHICSVIEW_H
#define PRIVATEQGRAPHICSVIEW_H

#include <QGraphicsView>

class PrivateQGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit PrivateQGraphicsView(QWidget *parent = 0);
    PrivateQGraphicsView(QGraphicsScene* scene, QWidget * parent=0);
    virtual ~PrivateQGraphicsView();

protected:
    //virtual from QGraphicsView
    virtual void mouseDoubleClickEvent(QMouseEvent *event);

    //virtual from QGraphicsView
    virtual void mouseMoveEvent(QMouseEvent *event);

    //virtual from QGraphicsView
    virtual void mousePressEvent(QMouseEvent *event);

    //virtual from QGraphicsView
    virtual void mouseReleaseEvent(QMouseEvent *event);

    //virtual from QGraphicsView
    virtual void contextMenuEvent(QContextMenuEvent *event);

    //virtual from QGraphicsView
    virtual void wheelEvent(QWheelEvent *event);
    
signals:
    void hadMouseDoubleClickEvent(QMouseEvent* event);
    void hadMouseMoveEvent(QMouseEvent * event);
    void hadMousePressEvent(QMouseEvent * event);
    void hadMouseReleaseEvent(QMouseEvent * event);
    void hadContextMenuEvent(QContextMenuEvent *);
    void hadWheelEvent(QWheelEvent *);
    
public slots:
    
};

#endif // PRIVATEQGRAPHICSVIEW_H
