//
// Copyright (C) 2017 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com.
// All rights reserved.
//
// This file is part of the KD HockeyApp library.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of either:
//
//   The GNU Lesser General Public License version 2.1 and version 3
//   as published by the Free Software Foundation and appearing in the
//   file LICENSE.LGPL.txt included.
//
// Or:
//
//   The Mozilla Public License Version 2.0 as published by the Mozilla
//   Foundation and appearing in the file LICENSE.MPL2.txt included.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Contact info@kdab.com if any conditions of this licensing is not clear to you.
//

#ifndef KDHOCKEYAPPMANAGER_H
#define KDHOCKEYAPPMANAGER_H

#include <QObject>
#include <QUrl>
#include <QVariantList>

class QNetworkAccessManager;
class QNetworkReply;
class QQmlEngine;

namespace KDHockeyApp {

struct HockeyAppVersionInfo
{
    Q_GADGET
    Q_PROPERTY(QString versionName MEMBER versionName CONSTANT FINAL)
    Q_PROPERTY(qint64 timestamp MEMBER timestamp CONSTANT FINAL)
    Q_PROPERTY(qint64 size MEMBER size CONSTANT FINAL)
    Q_PROPERTY(QString notes MEMBER notes CONSTANT FINAL)
    Q_PROPERTY(bool mandatory MEMBER mandatory CONSTANT FINAL)

public:
    QString versionName;
    qint64 timestamp;
    qint64 size;
    QString notes;
    bool mandatory;
};

class HockeyAppManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList newVersions READ newVersions NOTIFY newVersionsFound FINAL)

public:
    explicit HockeyAppManager(const QString &appId, QObject *parent = {});
    ~HockeyAppManager();

    void setNetworkAccessManager(QNetworkAccessManager *manager);
    QNetworkAccessManager *networkAccessManager() const;

    void setQmlEngine(QQmlEngine *engine);
    QQmlEngine *qmlEngine() const;

    QNetworkReply *uploadCrashDump(const QString &dumpFileName) const;
    void uploadCrashDumps() const;

    Q_INVOKABLE void findNewVersions();
    Q_INVOKABLE QUrl installUrl();

    QVariantList newVersions() const;

signals:
    void newVersionsFound(const QVariantList &newVersions);

private:
    struct AppInfo
    {
        QByteArray toByteArray() const;

        QString packageName;
        int versionCode = 0;
        QString versionName;
        QString platformId;
        QString platformVersion;
        QString platformBuild;
        QString vendorName;
        QString modelName;
        QString deviceId;
    };

    class Private;
    class PlatformPrivate;
    Private *const d;
};

QDebug operator<<(QDebug debug, const HockeyAppVersionInfo &);

} // namespace KDHockeyApp

#endif // KDHOCKEYAPPMANAGER_H
