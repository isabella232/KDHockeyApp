include(breakpad-config.pri)

TEMPLATE = lib
CONFIG += static

SOURCES += \
    breakpad_dummy.cpp

OTHER_FILES += \
    breakpad.pri \
    breakpad-config.pri

# Custom targets to build the breakpad library -----------------------------------------------------

android {
    breakpad_ndkbuild = +$$NDK_ROOT/ndk-build \
        APP_PLATFORM=$$ANDROID_PLATFORM APP_ABI=$$ANDROID_TARGET_ARCH APP_STL=gnustl_shared \
        APP_PROJECT_PATH=$$system_path($$PWD/src/android/sample_app) \
        NDK_TOOLCHAIN_VERSION=$$NDK_TOOLCHAIN_VERSION \
        TARGET_OUT=$$system_path($$BREAKPAD_BUILDDIR) \
        -C $$system_path($$PWD/src/android/sample_app)

    !silent: breakpad_ndkbuild += V=1

    breakpad_build.depends = $$_PRO_FILE_ $$files(src/client/linux/*.cc, true)
    breakpad_build.target = $$BREAKPAD_BUILDDIR/libbreakpad_client.a
    breakpad_build.commands = $$breakpad_ndkbuild

    breakpad_clean.commands = $$breakpad_ndkbuild clean

    QMAKE_EXTRA_TARGETS += breakpad_build breakpad_clean
    PRE_TARGETDEPS += $$breakpad_build.target
    CLEAN_DEPS += breakpad_clean
} else: linux: equals(QMAKE_HOST.os, Linux) {
    BREAKPAD_CPPFLAGS = $$QMAKE_CPPFLAGS

    for(incdir, QMAKE_INCDIR): BREAKPAD_CPPFLAGS += -I$$incdir

    BREAKPAD_WARNINGS += \
        -Wno-tautological-compare \
        -Wno-tautological-constant-out-of-range-compare \
        -Wno-unused-value

    BREAKPAD_CFLAGS = $$QMAKE_CFLAGS
    BREAKPAD_CXXFLAGS = $$QMAKE_CXXFLAGS

    CONFIG(debug, debug|release) {
        BREAKPAD_CFLAGS += $$QMAKE_CFLAGS_DEBUG
        BREAKPAD_CXXFLAGS += $$QMAKE_CXXFLAGS_DEBUG
    } else: CONFIG(release, debug|release) {
         BREAKPAD_CFLAGS += $$QMAKE_CFLAGS_RELEASE
         BREAKPAD_CXXFLAGS += $$QMAKE_CXXFLAGS_RELEASE
    }

    breakpad_configure.depends = $$_PRO_FILE_ $$files(src/client/linux/*.cc, true)
    breakpad_configure.target = $$BREAKPAD_BUILDDIR/Makefile
    breakpad_configure.commands = \
        mkdir -p $$BREAKPAD_BUILDDIR && \
        cd $$BREAKPAD_BUILDDIR && \
        $$PWD/src/configure \
            $$shell_quote(CC=$$QMAKE_CC) \
            $$shell_quote(CFLAGS=$$BREAKPAD_CFLAGS $$BREAKPAD_WARNINGS) \
            $$shell_quote(CPPFLAGS=$$BREAKPAD_CPPFLAGS) \
            $$shell_quote(CXX=$$QMAKE_CXX) \
            $$shell_quote(CXXFLAGS=$$BREAKPAD_CXXFLAGS $$BREAKPAD_WARNINGS) \
            $$shell_quote(LDFLAGS=$$QMAKE_LFLAGS)

    silent: breakpad_configure.commands += --enable-silent-rules

    breakpad_build.depends = breakpad_configure FORCE
    breakpad_build.target = $$BREAKPAD_BUILDDIR/src/client/linux/libbreakpad_client.a
    breakpad_build.commands = +\$(MAKE) -C $$BREAKPAD_BUILDDIR

    breakpad_clean.commands = +\$(MAKE) -C $$BREAKPAD_BUILDDIR clean
    breakpad_distclean.commands = +\$(MAKE) -C $$BREAKPAD_BUILDDIR distclean

    QMAKE_EXTRA_TARGETS += breakpad_build breakpad_clean breakpad_configure
    PRE_TARGETDEPS += $$breakpad_build.target
    DISTCLEAN_DEPS += breakpad_distclean
    CLEAN_DEPS += breakpad_clean
} else: ios: CONFIG(device, device|simulator) {
    BREAKPAD_SRCDIR=$$PWD/src/src/client/ios

    breakpad_build.target = $$BREAKPAD_BUILDDIR/libBreakpad.a
    breakpad_build.commands = xcodebuild \
        BUILD_DIR=$$OUT_PWD \
        CLANG_CXX_LIBRARY=libc++ \
        GCC_PREPROCESSOR_DEFINITIONS="USE_PROTECTED_ALLOCATIONS=0" \
        CONFIGURATION_BUILD_DIR=$$BREAKPAD_BUILDDIR \
        CONFIGURATION_TEMP_DIR=$$BREAKPAD_BUILDDIR \
        SHARED_PRECOMPS_DIR=$$OUT_PWD/PrecompiledHeaders \
        -sdk iphoneos -project $$BREAKPAD_SRCDIR/Breakpad.xcodeproj build

    QMAKE_EXTRA_TARGETS += breakpad_build
    PRE_TARGETDEPS += $$breakpad_build.target
    QMAKE_CLEAN += $$breakpad_build.target
} else {
    warning("Skipping breakpad, which is not supported for this platform")
}

# Custom targets to build breakpad's dump_syms tool ------------------------------------------------

equals(QMAKE_HOST.os, Linux) {
    dumpsyms_configure.depends = $$_PRO_FILE_
    dumpsyms_configure.target = $$BREAKPAD_HOSTDIR/Makefile
    dumpsyms_configure.commands = \
        mkdir -p $$BREAKPAD_HOSTDIR && \
        cd $$BREAKPAD_HOSTDIR && \
        $$PWD/src/configure

    dumpsyms_build.depends = dumpsyms_configure
    dumpsyms_build.target = $$BREAKPAD_HOSTDIR/src/tools/linux/dump_syms/dump_syms
    dumpsyms_build.commands = +\$(MAKE) -C $$BREAKPAD_HOSTDIR

    !equals(BREAKPAD_BUILDDIR, $$BREAKPAD_HOSTDIR) {
        QMAKE_EXTRA_TARGETS += dumpsyms_build dumpsyms_configure
        PRE_TARGETDEPS += $$dumpsyms_build.target
    }
}

# Install targets ----------------------------------------------------------------------------------

target.path = $$LIBDIR
target.extra = $$QMAKE_COPY $$BREAKPAD_LIBRARY $${QMAKE_PREFIX_STATICLIB}KDHockeyAppBreakpad.$${QMAKE_EXTENSION_STATICLIB}
target.files += $${QMAKE_PREFIX_STATICLIB}KDHockeyAppBreakpad.$${QMAKE_EXTENSION_STATICLIB}
INSTALLS += target
