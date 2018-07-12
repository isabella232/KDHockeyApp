!isEmpty(KDHOCKEYAPP_CONFIG): CONFIG = $$KDHOCKEYAPP_CONFIG

TEMPLATE = subdirs
VERSION = 0.1

SUBDIRS = src

skip_examples: KDHOCKEYAPP_SKIP *= examples
!contains(KDHOCKEYAPP_SKIP, "examples") {
    SUBDIRS += examples
    examples.depends += src
}

DOCUMENTS = \
    INSTALL.md \
    LICENSE.txt \
    LICENSE.LGPL.txt \
    LICENSE.MPL2.txt \
    README.md

QMAKE_SUBSTITUTES = \
    kdhockeyapp.pri.in \
    kdhockeyapp.pc.in

OTHER_FILES = \
    $$DOCUMENTS \
    $$QMAKE_SUBSTITUTES \
    tools/collectsymbols.sh

# Install targets ----------------------------------------------------------------------------------

isEmpty(PREFIX): PREFIX = /usr/local
cache(PREFIX, set)

isEmpty(LIBDIR): LIBDIR = $$PREFIX/lib
cache(LIBDIR, set)

isEmpty(INCLUDEDIR): INCLUDEDIR = $$PREFIX/include
cache(INCLUDEDIR, set)

isEmpty(DATAROOTDIR): DATAROOTDIR = $$PREFIX/share
cache(DATAROOTDIR)

isEmpty(DATADIR): DATADIR = $$DATAROOTDIR
cache(DATADIR)

isEmpty(DOCDIR): DOCDIR = $$DATAROOT/doc/kdhockeyapp
cache(DOCDIR)

docs.path = $$DOCDIR
docs.files = $$DOCUMENTS
INSTALLS += docs

mkspecs.path = $$LIBDIR/mkspecs
mkspecs.files = kdhockeyapp.pri
INSTALLS += mkspecs

pkgconfig.path = $$LIBDIR/pkgconfig
pkgconfig.files = kdhockeyapp.pc
INSTALLS += pkgconfig
