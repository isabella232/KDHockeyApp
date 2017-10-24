INCLUDEPATH += $$PWD
LIBS += -L$$shadowed($$PWD) -lKDHockeyApp
QT += network qml

android: QT += androidextras

include(../3rdparty/breakpad/breakpad.pri)
