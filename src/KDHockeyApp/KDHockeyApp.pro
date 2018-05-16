TEMPLATE = lib

CONFIG += no_private_qt_headers_warning static
CONFIG -= debug_and_release debug_and_release_target

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

OTHER_FILES += \
    KDHockeyApp.pri

# Install targets ----------------------------------------------------------------------------------

target.path = $$LIBDIR
INSTALLS += target

headers.path = $$INCLUDEDIR/KDHockeyApp
headers.files = $$INSTALLHEADERS
INSTALLS += headers
