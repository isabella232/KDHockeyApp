!isEmpty(KDHOCKEYAPP_CONFIG): CONFIG = $$KDHOCKEYAPP_CONFIG

TEMPLATE = app

QT += quickwidgets

include(../../src/KDHockeyApp/KDHockeyApp.pri)

RESOURCES = crashreports.qrc
SOURCES = crashreports.cpp
