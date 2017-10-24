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
#include "KDHockeyAppSoftAssert_p.h"

#include <client/linux/handler/exception_handler.h>

#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QStandardPaths>

namespace KDHockeyApp {

using google_breakpad::ExceptionHandler;
using google_breakpad::MinidumpDescriptor;

Q_DECLARE_LOGGING_CATEGORY(lcHockeyApp)

namespace {

class PlatformData
{
public:
    explicit PlatformData(const QString &path, ExceptionHandler::MinidumpCallback callback, void *context)
        : eh{MinidumpDescriptor{path.toStdString()}, nullptr, callback, context, true, -1}
    {}

    const ExceptionHandler eh;
};

} // namespace

// use multi-inheritance to ensure that the exception handler is initialized before all of the Private fields
class HockeyAppManager::PlatformPrivate : public PlatformData, public Private
{
public:
    explicit PlatformPrivate(const QString &appId, HockeyAppManager *q)
        : PlatformData{dataDirPath(), &PlatformPrivate::onException, this}
        , Private{appId, q}
    {}

private:
    static bool onException(const MinidumpDescriptor &, void *context, bool succeeded)
    {
        // NOTICE: This context is compromised. Complex operations, allocations must be avoided!
        return static_cast<const PlatformPrivate *>(context)->writeCrashReport(succeeded);
    }
};

HockeyAppManager::Private *HockeyAppManager::Private::create(const QString &appId, HockeyAppManager *q)
{
    return new PlatformPrivate{appId, q};
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
    return static_cast<const PlatformPrivate *>(this)->eh.minidump_descriptor().path();
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
