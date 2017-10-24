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

#ifdef __i386__
// This typedef is in the Android headers Breakpad bundles, but not in the
// headers bundled with the Anroid NDK. No idea who is correct, but in the
// end we should use the NDK headers. Therefore let's just have the typdef.
typedef struct user_fxsr_struct user_fpxregs_struct;
#endif

#include <client/linux/handler/exception_handler.h>

#include <QAndroidJniEnvironment>
#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QtAndroid>

namespace KDHockeyApp {

using google_breakpad::ExceptionHandler;
using google_breakpad::MinidumpDescriptor;

Q_DECLARE_LOGGING_CATEGORY(lcHockeyApp)

namespace {

QString settingsString(jclass settingsClass, jobject resolver, const char *fieldName)
{
    const auto field = QAndroidJniObject::getStaticObjectField<jstring>(settingsClass, fieldName);
    static constexpr char signature[] = "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;";
    return QAndroidJniObject::callStaticObjectMethod(settingsClass, "getString", signature, resolver, field.object()).toString();
}

QAndroidJniObject findContext()
{
    const auto activity = QtAndroid::androidActivity();
    if (activity.isValid())
        return activity;
    return QtAndroid::androidService();
}

QAndroidJniObject findContentResolver(QAndroidJniObject context)
{
    if (KD_SOFTASSERT_FAILED(lcHockeyApp, context.isValid()))
        return {};

    return context.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
}

QAndroidJniObject findPackageManager(QAndroidJniObject context)
{
    if (KD_SOFTASSERT_FAILED(lcHockeyApp, context.isValid()))
        return {};

    return context.callObjectMethod("getPackageManager", "()Landroid/content/pm/PackageManager;");
}

QAndroidJniObject packageName(QAndroidJniObject context)
{
    if (KD_SOFTASSERT_FAILED(lcHockeyApp, context.isValid()))
        return {};

    return context.callObjectMethod<jstring>("getPackageName");
}

QAndroidJniObject installerPackageName(QAndroidJniObject packageManager, QAndroidJniObject packageName)
{
    if (KD_SOFTASSERT_FAILED(lcHockeyApp, packageManager.isValid()))
        return {};

    return packageManager.callObjectMethod("getInstallerPackageName",
                                           "(Ljava/lang/String;)Ljava/lang/String;",
                                           packageName.object());
}

bool isEmpty(QAndroidJniObject str)
{
    return str.isValid() && str.callMethod<jboolean>("isEmpty");
}

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

void HockeyAppManager::Private::fillAppInfo(AppInfo *appInfo)
{
    const auto context = findContext();

    if (!context.isValid()) {
        qCWarning(lcHockeyApp, "Could not find Android context");
        return;
    }

    const auto contentResolver = findContentResolver(context);
    const auto packageManager = findPackageManager(context);

    QAndroidJniEnvironment jni;
    const auto buildClass = jni->FindClass("android/os/Build");
    const auto buildVersionClass = jni->FindClass("android/os/Build$VERSION");
    const auto settingsClass = jni->FindClass("android/provider/Settings$Secure");

    const auto rawDeviceId = settingsString(settingsClass, contentResolver.object(), "ANDROID_ID").toUtf8();
    appInfo->deviceId = QString::fromLatin1(QCryptographicHash::hash(rawDeviceId, QCryptographicHash::Sha256).toHex());

    appInfo->platformId = "Android"_l1;
    appInfo->platformVersion = QAndroidJniObject::getStaticObjectField<jstring>(buildVersionClass, "RELEASE").toString();
    appInfo->platformBuild = QAndroidJniObject::getStaticObjectField<jstring>(buildClass, "DISPLAY").toString();
    appInfo->modelName = QAndroidJniObject::getStaticObjectField<jstring>(buildClass, "MODEL").toString();
    appInfo->vendorName = QAndroidJniObject::getStaticObjectField<jstring>(buildClass, "MANUFACTURER").toString();

    const auto packageInfo = packageManager.callObjectMethod("getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;",
                                                             packageName(context).object(), 0);

    if (!jni->ExceptionCheck()) {
        appInfo->packageName = packageInfo.getObjectField<jstring>("packageName").toString();
        appInfo->versionName = packageInfo.getObjectField<jstring>("versionName").toString();
        appInfo->versionCode = packageInfo.getField<jint>("versionCode");
    } else {
        jni->ExceptionDescribe();
        jni->ExceptionClear();
    }
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
    const auto context = findContext();
    const auto packageManager = findPackageManager(context);
    const auto installer = installerPackageName(packageManager, packageName(context).object());

    QAndroidJniEnvironment jni;

    if (jni->ExceptionCheck()) {
        jni->ExceptionDescribe();
        jni->ExceptionClear();
        return false;
    }

    return !isEmpty(installer);
}

} // namespace KDHockeyApp
