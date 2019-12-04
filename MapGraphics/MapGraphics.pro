#-------------------------------------------------
#
# Project created by QtCreator 2012-03-03T10:50:47
#
#-------------------------------------------------

QT       += network sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MapGraphics
TEMPLATE = lib
CONFIG += staticlib

DEFINES += MAPGRAPHICS_LIBRARY

INCLUDEPATH += .

SOURCES += MapGraphicsScene.cpp \
    MapGraphicsObject.cpp \
    MapGraphicsView.cpp \
    guts/PrivateQGraphicsScene.cpp \
    guts/PrivateQGraphicsObject.cpp \
    guts/Conversions.cpp \
    MapTileSource.cpp \
    tileSources/GridTileSource.cpp \
    guts/MapTileGraphicsObject.cpp \
    guts/PrivateQGraphicsView.cpp \
    tileSources/OSMTileSource.cpp \
    guts/MapGraphicsNetwork.cpp \
    tileSources/CompositeTileSource.cpp \
    guts/MapTileLayerListModel.cpp \
    guts/MapTileSourceDelegate.cpp \
    guts/CompositeTileSourceConfigurationWidget.cpp \
    CircleObject.cpp \
    guts/PrivateQGraphicsInfoSource.cpp \
    PolygonObject.cpp \
    Position.cpp \
    LineObject.cpp

HEADERS += MapGraphicsScene.h\
        MapGraphics_global.h \
    MapGraphicsObject.h \
    MapGraphicsView.h \
    guts/PrivateQGraphicsScene.h \
    guts/PrivateQGraphicsObject.h \
    guts/Conversions.h \
    MapTileSource.h \
    tileSources/GridTileSource.h \
    guts/MapTileGraphicsObject.h \
    guts/PrivateQGraphicsView.h \
    tileSources/OSMTileSource.h \
    guts/MapGraphicsNetwork.h \
    tileSources/CompositeTileSource.h \
    guts/MapTileLayerListModel.h \
    guts/MapTileSourceDelegate.h \
    guts/CompositeTileSourceConfigurationWidget.h \
    CircleObject.h \
    guts/PrivateQGraphicsInfoSource.h \
    PolygonObject.h \
    Position.h \
    LineObject.h

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE4F7F973
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = MapGraphics.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

FORMS += \
    guts/CompositeTileSourceConfigurationWidget.ui

RESOURCES += \
    resources.qrc
