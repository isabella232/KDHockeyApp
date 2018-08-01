android {
    isEmpty(BREAKPAD_HOSTDIR): BREAKPAD_HOSTDIR = $$shadowed($$PWD/build-host)
    TARGET_FILENAME = lib$${TARGET}.so
} else: ios {
    CONFIG(release, debug|release) {
        BREAKPAD_CONFIG = Release
    } else {
        BREAKPAD_CONFIG = Debug
    }

    BREAKPAD_BUILDDIR = $$shadowed($${BREAKPAD_CONFIG}-iphoneos)
    TARGET_FILENAME = $${BREAKPAD_CONFIG}-iphoneos/$${TARGET}.app/$${TARGET}
} else {
    TARGET_FILENAME = $$TARGET
}

# DESTDIR on iOS is to be avoided as it's not really working correctly.
!ios: !isEmpty(DESTDIR): TARGET_FILENAME = $$DESTDIR/$$TARGET_FILENAME

isEmpty(BREAKPAD_BUILDDIR): BREAKPAD_BUILDDIR = $$shadowed($$PWD/build-target)
isEmpty(BREAKPAD_HOSTDIR): BREAKPAD_HOSTDIR = $$BREAKPAD_BUILDDIR

BREAKPAD_LIBRARY = $$shadowed($$PWD/$${QMAKE_PREFIX_STATICLIB}breakpad.$${QMAKE_EXTENSION_STATICLIB})
