include(breakpad-config.pri)

INCLUDEPATH += $$PWD/src/src

!isEqual(TEMPLATE, lib)|CONFIG(shared, static|shared) {
    LIBS -= $$BREAKPAD_LIBRARY
    LIBS += $$BREAKPAD_LIBRARY
    PRE_TARGETDEPS += $$BREAKPAD_LIBRARY
}

equals(TEMPLATE, app) {
    android: equals(QMAKE_HOST.os, Linux) {
        dumpsyms.commands = \
            $$shell_quote(CXX=$$QMAKE_CXX) \
            $$shell_quote(DUMPSYMS=$$BREAKPAD_HOSTDIR/src/tools/linux/dump_syms/dump_syms) \
            $$shell_quote(SYSROOT=$$ANDROID_PLATFORM_ROOT_PATH) \
            $$clean_path($$PWD/../../../tools/collectsymbols.sh) $$TARGET_FILENAME || \
            { rm $$TARGET_FILENAME; false; }
    }

    ios: CONFIG(device, device|simulator) {
        dumpsyms.commands = \
            dsymutil $${TARGET_FILENAME} -o $${TARGET}.dSYM && \
            zip -rm $${TARGET}.dSYM.zip $${TARGET}.dSYM/ || \
            { rm $${TARGET_FILENAME}; false; }
    }

    !isEmpty(dumpsyms.commands): CONFIG(release, debug|release) {
        QMAKE_EXTRA_TARGETS += dumpsyms
        QMAKE_POST_LINK += $$escape_expand(\\n\\t)$$dumpsyms.commands
    }
}
