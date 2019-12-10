#ifndef APRSCONTROLLER_H
#define APRSCONTROLLER_H

#include <QObject>
#include <MapGraphics/MapGraphicsScene.h>
#include <MapGraphics/MapGraphicsView.h>
#include <MapGraphics/guts/CompositeTileSourceConfigurationWidget.h>
#include <MapGraphics/tileSources/OSMTileSource.h>
#include <MapGraphics/tileSources/GridTileSource.h>
#include <MapGraphics/tileSources/CompositeTileSource.h>
#include "aprs.h"
#include "aprsstation.h"
#include "aprspixmapitem.h"
#include "ui_mainwindow.h"
#include "src/settings.h"

class AprsController : public QObject
{
    Q_OBJECT
public:
    explicit AprsController(Settings *settings, Ui::MainWindow *wui, QObject *parent = nullptr);
    static QPointF convertToLL(QPointF pos, double zoom);
    static QPointF convertToXY(QPointF ll, double zoom);

    void connectToAPRS();
    void showRawAPRSMessages();
    void changeAPRSTimeFilter(int hours);
    void filterPrefixAPRS();
    void clearFilterPrefixAPRS();
signals:


public slots:
    void newAPRSquery(quint8 zoom);
    void activateAPRS(bool active);
    //void setReceived(QString data);
    void processAPRSData(AprsStation *st);
    void processRawAPRSData(QString data);
    void clearAPRS();
    void restoreMapState();
    void setMapItems(quint8 zoom);

private:
    Ui::MainWindow *ui;
    Aprs *_aprs;
    Settings *_settings;
    MapGraphicsScene * _map_scene;
    MapGraphicsView * _map_view;
    QMap<AprsPixmapItem *, AprsIcon> _map_aprs;
    QMap<QGraphicsTextItem *, QPointF> _map_aprs_text;
    QVector<QString *> _raw_aprs_messages;
    QVector<AprsStation*> _aprs_stations;
    QVector<QGraphicsLineItem*> _aprs_lines;
    typedef QVector<QPointF> draw_lines;
    QMap<QString,draw_lines> _moving_stations;
    bool _zoom_aprs_filter_distance;


};

#endif // APRSCONTROLLER_H
