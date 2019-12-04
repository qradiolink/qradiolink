TEMPLATE = subdirs

SUBDIRS += MapGraphics \
    application

application.depends += MapGraphics

