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

!isEmpty(DESTDIR): TARGET_FILENAME = $$DESTDIR/$$TARGET_FILENAME

isEmpty(BREAKPAD_BUILDDIR): BREAKPAD_BUILDDIR = $$shadowed($$PWD/build-target)
isEmpty(BREAKPAD_HOSTDIR): BREAKPAD_HOSTDIR = $$BREAKPAD_BUILDDIR

android {
    BREAKPAD_LIBRARY = $$system_path($$BREAKPAD_BUILDDIR/libbreakpad_client.a)
} else: linux {
    BREAKPAD_LIBRARY = $$system_path($$BREAKPAD_BUILDDIR/src/client/linux/libbreakpad_client.a)
} else: ios: CONFIG(device, device|simulator) {
    BREAKPAD_LIBRARY = $$system_path($$BREAKPAD_BUILDDIR/libBreakpad.a)
}
