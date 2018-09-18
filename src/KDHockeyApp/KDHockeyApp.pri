KDHOCKEYAPP_LIBRARY_FILENAME = $$shadowed($$PWD/$${QMAKE_PREFIX_STATICLIB}KDHockeyApp.$${QMAKE_EXTENSION_STATICLIB})

INCLUDEPATH += $$system_path($$shadowed(include))
QT += network qml

android: QT += androidextras

!isEqual(TEMPLATE, lib)|CONFIG(shared, static|shared) {
    LIBS -= $$KDHOCKEYAPP_LIBRARY_FILENAME
    LIBS += $$KDHOCKEYAPP_LIBRARY_FILENAME
    PRE_TARGETDEPS *= $$KDHOCKEYAPP_LIBRARY_FILENAME
}

include(../3rdparty/breakpad/breakpad.pri)
