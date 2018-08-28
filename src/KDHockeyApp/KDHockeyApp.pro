!isEmpty(KDHOCKEYAPP_CONFIG): CONFIG = $$KDHOCKEYAPP_CONFIG

TEMPLATE = lib

CONFIG += no_private_qt_headers_warning static
CONFIG -= debug_and_release debug_and_release_target

INCLUDEPATH += $$system_path($$shadowed(include))

QT = network qml-private

include(../3rdparty/breakpad/breakpad.pri)

INSTALL_HEADERS = \
    KDHockeyAppManager.h

HEADERS = \
    $$INSTALL_HEADERS \
    KDHockeyAppLiterals_p.h \
    KDHockeyAppManager_p.h \
    KDHockeyAppSoftAssert_p.h

SOURCES = \
    KDHockeyAppManager.cpp \
    KDHockeyAppSoftAssert.cpp

android {
    QT += androidextras

    SOURCES += \
        KDHockeyAppManager_android.cpp
} else: ios: CONFIG(device, device|simulator) {
    OBJECTIVE_SOURCES += \
        KDHockeyAppManager_ios.mm
} else: linux: {
    SOURCES += \
        KDHockeyAppManager_linux.cpp
} else {
    SOURCES += \
        KDHockeyAppManager_generic.cpp
}

KDHockeyAppManager.config = verbatim
KDHockeyAppManager.input = KDHockeyAppManager.h
KDHockeyAppManager.output = include/KDHockeyAppManager.h

QMAKE_SUBSTITUTES += \
    KDHockeyAppManager

DISTFILES += \
    KDHockeyApp.pri \
    $$KDHockeyAppManager.input

# Install targets ----------------------------------------------------------------------------------

target.path = $$LIBDIR
INSTALLS += target

headers.path = $$INCLUDEDIR/KDHockeyApp
headers.files = $$INSTALLHEADERS
INSTALLS += headers
