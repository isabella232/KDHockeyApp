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

#ifndef KDHOCKEYAPPMANAGER_P_H
#define KDHOCKEYAPPMANAGER_P_H

#include "KDHockeyAppManager.h"

#include <QContiguousCache>
#include <QDir>
#include <QPointer>

namespace KDHockeyApp {

class HockeyAppManager::Private
{
protected:
    explicit Private(const QString &appId, HockeyAppManager *q)
        : appId{appId}
        , q{q}
    {}

public:
    virtual ~Private() = default;

    static Private *create(const QString &appId, HockeyAppManager *q);

    bool writeCrashReport(bool miniDumpWritten) const;
    bool writeLogFile(const QContiguousCache<char> &logBuffer) const;
    bool writeMetaFile() const;
    bool writeQmlTrace() const;

    static QDir cacheLocation();
    static QString dataDirPath();
    static AppInfo makeAppInfo();
    static void fillAppInfo(AppInfo *appInfo);
    std::string makeCrashFileName(const std::string &suffix) const;
    std::string nextMiniDumpFileName() const;

    static QString requestParameterDeviceId();
    static bool installedFromMarket();

    void setNetworkAccessManager(QNetworkAccessManager *network);
    QNetworkAccessManager *networkAccessManager();

    const QString appId;
    const QByteArray metaData{makeAppInfo().toByteArray()};
    const std::string logFileName{makeCrashFileName("log")};
    const std::string metaFileName{makeCrashFileName("dsc")};
    const std::string qmlTraceFileName{makeCrashFileName("qst")};

    QVariantList newVersions;
    QPointer<QQmlEngine> qmlEngine;
    HockeyAppManager *const q;

private:
    QPointer<QNetworkAccessManager> m_network;
};

} // namespace KDHockeyApp

#endif // KDHOCKEYAPPMANAGER_P_H
