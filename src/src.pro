TEMPLATE = subdirs

SUBDIRS = \
    3rdparty \
    KDHockeyApp

KDHockeyApp.depends += 3rdparty
