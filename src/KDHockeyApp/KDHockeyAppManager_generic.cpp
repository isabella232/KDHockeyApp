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

#include "KDHockeyAppManager_p.h"

#include "KDHockeyAppLiterals_p.h"

#include <QStandardPaths>

namespace KDHockeyApp {

HockeyAppManager::Private *HockeyAppManager::Private::create(const QString &appId, HockeyAppManager *q)
{
    return new Private{appId, q};
}

QDir HockeyAppManager::Private::cacheLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

void HockeyAppManager::Private::fillAppInfo(AppInfo *)
{
}

std::string HockeyAppManager::Private::nextMiniDumpFileName() const
{
    Q_UNIMPLEMENTED();
    return {};
}

QString HockeyAppManager::Private::requestParameterDeviceId()
{
    return "udid"_l1;
}

bool HockeyAppManager::Private::installedFromMarket()
{
    return false;
}

} // namespace KDHockeyApp
