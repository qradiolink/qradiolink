#-------------------------------------------------
#
# Project created by QtCreator 2012-03-03T10:50:47
#
#-------------------------------------------------

QT       += network sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


DEFINES += MAPGRAPHICS_LIBRARY

INCLUDEPATH += $$PWD

SOURCES += MapGraphics/MapGraphicsScene.cpp \
    MapGraphics/MapGraphicsObject.cpp \
    MapGraphics/MapGraphicsView.cpp \
    MapGraphics/guts/PrivateQGraphicsScene.cpp \
    MapGraphics/guts/PrivateQGraphicsObject.cpp \
    MapGraphics/guts/Conversions.cpp \
    MapGraphics/MapTileSource.cpp \
    MapGraphics/tileSources/GridTileSource.cpp \
    MapGraphics/guts/MapTileGraphicsObject.cpp \
    MapGraphics/guts/PrivateQGraphicsView.cpp \
    MapGraphics/tileSources/OSMTileSource.cpp \
    MapGraphics/guts/MapGraphicsNetwork.cpp \
    MapGraphics/tileSources/CompositeTileSource.cpp \
    MapGraphics/guts/MapTileLayerListModel.cpp \
    MapGraphics/guts/MapTileSourceDelegate.cpp \
    MapGraphics/guts/CompositeTileSourceConfigurationWidget.cpp \
    MapGraphics/CircleObject.cpp \
    MapGraphics/guts/PrivateQGraphicsInfoSource.cpp \
    MapGraphics/PolygonObject.cpp \
    MapGraphics/Position.cpp \
    MapGraphics/LineObject.cpp

HEADERS += MapGraphics/MapGraphicsScene.h\
        MapGraphics/MapGraphics_global.h \
    MapGraphics/MapGraphicsObject.h \
    MapGraphics/MapGraphicsView.h \
    MapGraphics/guts/PrivateQGraphicsScene.h \
    MapGraphics/guts/PrivateQGraphicsObject.h \
    MapGraphics/guts/Conversions.h \
    MapGraphics/MapTileSource.h \
    MapGraphics/tileSources/GridTileSource.h \
    MapGraphics/guts/MapTileGraphicsObject.h \
    MapGraphics/guts/PrivateQGraphicsView.h \
    MapGraphics/tileSources/OSMTileSource.h \
    MapGraphics/guts/MapGraphicsNetwork.h \
    MapGraphics/tileSources/CompositeTileSource.h \
    MapGraphics/guts/MapTileLayerListModel.h \
    MapGraphics/guts/MapTileSourceDelegate.h \
    MapGraphics/guts/CompositeTileSourceConfigurationWidget.h \
    MapGraphics/CircleObject.h \
    MapGraphics/guts/PrivateQGraphicsInfoSource.h \
    MapGraphics/PolygonObject.h \
    MapGraphics/Position.h \
    MapGraphics/LineObject.h



FORMS += \
    MapGraphics/guts/CompositeTileSourceConfigurationWidget.ui

RESOURCES += \
    MapGraphics/map_resources.qrc
