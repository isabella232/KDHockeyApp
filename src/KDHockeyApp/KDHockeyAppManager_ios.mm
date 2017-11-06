// Copyright (C) 2017 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com.
// All rights reserved.
//
// This file is part of the KD HockeyApp library.
//
// This file may be distributed and/or modified under the terms of the
// GNU Lesser General Public License version 2.1 and version 3 as published
// by the Free Software Foundation and appearing in the file LICENSE.LGPL.txt
// included.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Contact info@kdab.com if any conditions of this licensing are not
// clear to you.

#include "KDHockeyAppManager_p.h"

#include "KDHockeyAppLiterals_p.h"

#include <client/ios/exception_handler_no_mach.h>

#include <QCoreApplication>

#import <Foundation/Foundation.h>

#include <mach-o/dyld.h>
#include <sys/utsname.h>

namespace KDHockeyApp {

namespace {

using google_breakpad::ExceptionHandler;

QByteArray executableUuid()
{
    if (const auto executableHeader = []() -> const mach_header * {
        for (quint32 i = 0; i < _dyld_image_count(); i++) {
            const auto header = _dyld_get_image_header(i);
            if (header->filetype == MH_EXECUTE)
                return header;
        }

        return nullptr;
    }()) {
        const auto is64bit = (executableHeader->magic == MH_MAGIC_64 || executableHeader->magic == MH_CIGAM_64);
        const auto headerSize = (is64bit ? sizeof(struct mach_header_64) : sizeof(struct mach_header));
        auto cursor = reinterpret_cast<quintptr>(executableHeader) + headerSize;

        const segment_command *segmentCommand = {};
        for (quint32 i = 0; i < executableHeader->ncmds; i++, cursor += segmentCommand->cmdsize) {
            segmentCommand = reinterpret_cast<const segment_command *>(cursor);
            if (segmentCommand->cmd == LC_UUID) {
                const auto uuidCommand = reinterpret_cast<const uuid_command *>(segmentCommand);
                return {reinterpret_cast<const char *>(uuidCommand->uuid), 16};
            }
        }
    }

    return {};
}

QString pathForDirectory(NSSearchPathDirectory directory, NSSearchPathDomainMask mask)
{
    return QString::fromNSString([NSSearchPathForDirectoriesInDomains(directory, mask, YES) lastObject]);
}

QString appendOrganizationAndApplicationName(QString &&path)
{
    if (!qApp->organizationName().isEmpty())
        path += '/'_l1 + qApp->organizationName();
    if (!qApp->applicationName().isEmpty())
        path += '/'_l1 + qApp->applicationName();

    return path;
}

class PlatformData
{
public:
    explicit PlatformData(const QString &path, ExceptionHandler::MinidumpCallback callback, void *context)
        : eh{path.toStdString(), nullptr, callback, context, true, nullptr}
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
    static bool onException(const char *, const char *, void *context, bool succeeded)
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
    return appendOrganizationAndApplicationName(pathForDirectory(NSCachesDirectory, NSUserDomainMask));
}

void HockeyAppManager::Private::fillAppInfo(AppInfo *appInfo)
{
    utsname sysinfo;
    uname(&sysinfo);

    appInfo->platformId = "iOS"_l1;
    appInfo->modelName = QString::fromUtf8(sysinfo.machine);
    appInfo->vendorName = "Apple"_l1;
    appInfo->deviceId = QString::fromLatin1(executableUuid().toHex());
}

std::string HockeyAppManager::Private::nextMiniDumpFileName() const
{
    return static_cast<const PlatformPrivate *>(this)->eh.next_minidump_path();
}

QString HockeyAppManager::Private::requestParameterDeviceId()
{
    return "uuid"_l1;
}

bool HockeyAppManager::Private::installedFromMarket()
{
    // MobilePovision profiles are a clear indicator for Ad-Hoc distribution
    if ([[NSBundle mainBundle] pathForResource:@"embedded" ofType:@"mobileprovision"])
        return false;

    // TestFlight is only supported from iOS 8 onwards, so at this point we have to be in the AppStore
    if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_6_1)
        return true;

    // Check if App Store receipt points points to a TestFlight sandbox
    if ([NSBundle.mainBundle respondsToSelector:@selector(appStoreReceiptURL)]) {
        const auto appStoreReceiptURL = NSBundle.mainBundle.appStoreReceiptURL;
        const auto appStoreReceiptLastComponent = appStoreReceiptURL.lastPathComponent;
        if ([appStoreReceiptLastComponent isEqualToString:@"sandboxReceipt"])
            return false;
    }

    return true;
}

} // namespace KDHockeyApp
