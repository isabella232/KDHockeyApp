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

#include <private/qqmlengine_p.h>

#include <QFile>
#include <QDirIterator>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QSettings>
#include <QUrlQuery>
#include <QVersionNumber>

#include <functional>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef Q_CC_MSVC
#include <io.h>
#else
#include <unistd.h>
#endif

extern "C" Q_QML_EXPORT char *qt_v4StackTrace(void *);

namespace KDHockeyApp {

Q_LOGGING_CATEGORY(lcHockeyApp, "kdab.kdhockeyapp")

namespace {

enum Necessity { Mandatory, Optional };

constexpr int logCapacity = 32768;
QContiguousCache<char> logBuffer{logCapacity};
QContiguousCache<char> lockedDownLogBuffer{logCapacity}; // to prevent races while writing its contents to disk
char writeBuffer[logCapacity];

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);
const auto defaultMessageHandler = qInstallMessageHandler(messageHandler);

const auto s_formDataHeader = QStringLiteral(R"(form-data; name="%1"; filename="%2")");
const auto s_formDataHeaderMeta = QStringLiteral(R"(form-data; name="log"; filename="%1.meta")");
const auto s_restUrlUploadCrashReport =QStringLiteral("https://rink.hockeyapp.net/api/2/apps/%1/crashes/upload");
const auto s_restUrlListVersions = QStringLiteral("https://rink.hockeyapp.net/api/2/apps/%1");
const auto s_settingsStartTime = QStringLiteral("HockeyApp/Usage/StartTime");
const auto s_settingsUsageDuration = QStringLiteral("HockeyApp/Usage/Duration");
const auto s_webUrlInstallPage = QStringLiteral("https://rink.hockeyapp.net/apps/%1");

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QString tmp;

    switch (type) {
    case QtDebugMsg:
        tmp += "[D] "_l1;
        break;
    case QtWarningMsg:
        tmp += "[W] "_l1;
        break;
    case QtCriticalMsg:
        tmp += "[C] "_l1;
        break;
    case QtFatalMsg:
        tmp += "[F] "_l1;
        break;
    case QtInfoMsg:
        tmp += "[I] "_l1;
        break;
    }

    if (context.category) {
        tmp.append(QString::fromLatin1(context.category));
        tmp.append(": "_l1);
    }

    if (context.function) {
        tmp.append(QString::fromLatin1(context.function));

        if (context.line > 0) {
            tmp.append(", line "_l1);
            tmp.append(QString::number(context.line));
        }

        tmp.append(": "_l1);
    }

    tmp.append(message);
    tmp.append('\n'_l1);

    const auto utf8Data = tmp.toUtf8();
    for (auto ch: utf8Data)
        logBuffer.append(ch);

    logBuffer.normalizeIndexes(); // avoid that indexes overflow
    defaultMessageHandler(type, context, message);
}

bool attachFile(QHttpMultiPart *formData, const QString &fieldName,
                const QString &fileName, QStringList *crashFiles, Necessity necessity)
{
    QScopedPointer<QFile> file{new QFile{fileName, formData}};

    if (!file->open(QFile::ReadOnly)) {
        if (!file->exists() && necessity == Optional)
            return true;

        qCWarning(lcHockeyApp, "Could not open %ls to upload crash data: %ls",
                  qUtf16Printable(file->fileName()), qUtf16Printable(file->errorString()));
        return false;
    }

    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   s_formDataHeader.arg(fieldName, QFileInfo(fileName).fileName()));
    part.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream"_l1);
    part.setBodyDevice(file.take());

    formData->append(part);
    crashFiles->append(fileName);

    return true;
}

qint64 usageDuration(const QSettings &settings = QSettings{})
{
    return settings.value(s_settingsUsageDuration).toLongLong();
}

template<class Engine> auto engineContext(const Engine *engine) -> decltype(engine->currentContext())
{
    return engine->currentContext();
}

template<class Engine> auto engineContext(const Engine *engine, ...) -> decltype(engine->currentContext)
{
    return engine->currentContext;
}

} // namespace

QByteArray HockeyAppManager::AppInfo::toByteArray() const
{
    QByteArray appInfo;

    QTextStream(&appInfo)
            << "Package: " << packageName << endl
            << "Version Code: " << versionCode << endl
            << "Version Name: " << versionName << endl
#ifdef Q_OS_ANDROID
            << "Android: " << platformVersion << endl
            << "Android Build: " << platformBuild << endl
#else
            << "OS Version: " << platformVersion << endl
#endif
            << "Manufacturer: " << vendorName << endl
            << "Model: " << modelName << endl
            << "Start Date: " << QDateTime::currentDateTime().toString(Qt::RFC2822Date) << endl
            << "Date: @@MINIDUMP_TIMESTAMP@@" << endl
            << "CrashReporter Key: @@CRASHID@@" << endl
            << endl
            << "MinidumpContainer" << endl;

    return appInfo;
}

bool HockeyAppManager::Private::writeLogFile(const QContiguousCache<char> &logBuffer) const
{
    // NOTICE: This context is compromised. Complex operations, allocations must be avoided!

    const auto size = logBuffer.size();

    // FIXME: I wish QContiguousCache would give direct access to its data
    for (auto i = 0; i < size; ++i)
        writeBuffer[i] = logBuffer.at(logBuffer.firstIndex() + i);

    const auto fd = open(logFileName.c_str(), O_CREAT | O_WRONLY, 0600);

    if (fd == -1)
        return false;

    const auto written = write(fd, writeBuffer, static_cast<size_t>(size));
    close(fd);

    return written == size;
}

bool HockeyAppManager::Private::writeMetaFile() const
{
    // NOTICE: This context is compromised. Complex operations, allocations must be avoided!

    const auto fd = open(metaFileName.c_str(), O_CREAT | O_WRONLY, 0600);

    if (fd == -1)
        return false;

    const auto written = write(fd, metaData.constData(), static_cast<size_t>(metaData.size()));
    close(fd);

    return written == metaData.size();
}

bool HockeyAppManager::Private::writeQmlTrace() const
{
    // NOTICE: This context is compromised. Complex operations, allocations must be avoided!

    if (const auto engine = qmlEngine ? QQmlEnginePrivate::getV4Engine(qmlEngine) : nullptr) {
        const auto stackTrace = qt_v4StackTrace(engineContext(engine)); // FIXME: we should not allocate memory here!

        if (!stackTrace)
            return false;

        const auto fd = open(qmlTraceFileName.c_str(), O_CREAT | O_WRONLY, 0600);

        if (fd == -1)
            return false;

        const auto toWrite = strlen(stackTrace);
        const auto written = write(fd, stackTrace, toWrite);
        close(fd);

        return written == static_cast<int>(toWrite);
    }

    return true;
}

bool HockeyAppManager::Private::writeCrashReport(bool minidumpWritten) const
{
    if (!minidumpWritten) {
        qCWarning(lcHockeyApp, "No mini dump was written, skipping this crash report");
        return false;
    }

    // NOTICE: This context is compromised. Complex operations, allocations must be avoided!
    logBuffer.swap(lockedDownLogBuffer); // prevent any writes while we are dumping it

    return writeMetaFile()
            && writeLogFile(lockedDownLogBuffer)
            && writeQmlTrace();
}

QString HockeyAppManager::Private::dataDirPath()
{
    return cacheLocation().filePath("crashes"_l1);
}

HockeyAppManager::AppInfo HockeyAppManager::Private::makeAppInfo()
{
    AppInfo appInfo;
    fillAppInfo(&appInfo);

    if (appInfo.platformVersion.isEmpty())
        appInfo.platformVersion = QSysInfo::productType() + ' '_l1 + QSysInfo::productVersion();
    if (appInfo.platformBuild.isEmpty())
        appInfo.platformBuild = QSysInfo::kernelType() + ' '_l1 + QSysInfo::kernelVersion();
    if (appInfo.packageName.isEmpty())
        appInfo.packageName = qApp->applicationName();
    if (appInfo.versionName.isEmpty())
        appInfo.versionName = qApp->applicationVersion();

    return appInfo;
}

std::string HockeyAppManager::Private::makeCrashFileName(const std::string &suffix) const
{
    std::string fileName{nextMiniDumpFileName()};

    if (fileName.length() < 40) {
        qCWarning(lcHockeyApp, "Invalid dump file name: %s", fileName.c_str());
        return {};
    }

    fileName.replace(fileName.length() - 3, 3, suffix);
    return fileName;
}

void HockeyAppManager::Private::setNetworkAccessManager(QNetworkAccessManager *network)
{
    if (m_network != network) {
        if (m_network && m_network->parent() == q)
            delete m_network;

        m_network = network;
    }
}

QNetworkAccessManager *HockeyAppManager::Private::networkAccessManager()
{
    if (m_network.isNull())
        m_network = new QNetworkAccessManager{q};

    return m_network;
}

/*!
    \class HockeyAppManager
    \brief Manager for creating crash reports and uploading

    On construction a breakpad crash handler is installed. Whenever the
    application crashes a crash report is written into the applications
    cache location. Once reports from previous crashes are succesfully
    uploaded via uploadCrashDumps() they are deleted.

    \note It is recommended to set applicationName and/or organizationName
    of the QCoreApplication instance so that crash reports are written to
    a reasonable place.
*/

/*!
    \property HockeyAppManager::newVersions
    \brief the list of available new versions

    This property is updated after every call to findNewVersions() and
    contains a list of HockeyAppVersionInfo elements representing new
    available versions of the application.

    \sa HockeyAppVersionInfo, findNewVersions()
*/

HockeyAppManager::HockeyAppManager(const QString &appId, QObject *parent)
    : QObject{parent}
    , d{Private::create(appId, this)}
{
    if (appId.isEmpty())
        qCWarning(lcHockeyApp, "Non-empty application id required");

    const QFileInfo crashDir{d->dataDirPath()};

    if (!crashDir.exists())
        QDir::current().mkpath(crashDir.filePath());
    if (!crashDir.isWritable())
        qCWarning(lcHockeyApp, "Crash dump directory doesn't seem writable: %ls", qUtf16Printable(d->dataDirPath()));

    // start usage time tracking
    QSettings{}.setValue(s_settingsStartTime, QDateTime::currentMSecsSinceEpoch());
    connect(qApp, &QCoreApplication::aboutToQuit, this, [] {
        QSettings settings;

        const auto startTime = settings.value(s_settingsStartTime).toLongLong();

        if (startTime > 0) {
            const auto currentUsage = QDateTime::currentMSecsSinceEpoch() - startTime;
            settings.setValue(s_settingsUsageDuration, usageDuration(settings) + currentUsage);
        }
    });
}

HockeyAppManager::~HockeyAppManager()
{
    delete d;
}


/*!
    \fn void HockeyAppManager::setNetworkAccessManager(QNetworkAccessManager *manager)

    Set the QNetworkAccessManager to use for network requests. If none is set
    by the user a new QNetworkAccessManager is created automatically.
*/

void HockeyAppManager::setNetworkAccessManager(QNetworkAccessManager *manager)
{
    d->setNetworkAccessManager(manager);
}

QNetworkAccessManager *HockeyAppManager::networkAccessManager() const
{
    return d->networkAccessManager();
}


/*!
    \fn void HockeyAppManager::setQmlEngine(QQmlEngine *engine)

    If a QML engine is set the crash handler will use it to generate
    a QML stack trace and add it to the crash report.
*/

void HockeyAppManager::setQmlEngine(QQmlEngine *engine)
{
    d->qmlEngine = engine;
}

QQmlEngine *HockeyAppManager::qmlEngine() const
{
    return d->qmlEngine;
}

/*!
    \fn void HockeyAppManager::uploadCrashDumps() const

    Upload all previously written crash reports to HockeyApp.
*/

void HockeyAppManager::uploadCrashDumps() const
{
    qCInfo(lcHockeyApp, "Searching for crashdumps in %ls", qUtf16Printable(d->dataDirPath()));
    for (QDirIterator it{d->dataDirPath(), {"*.dmp"_l1}}; it.hasNext(); )
        uploadCrashDump(it.next());
}

QNetworkReply *HockeyAppManager::uploadCrashDump(const QString &dumpFileName) const
{
    const auto commonFileName = dumpFileName.left(dumpFileName.length() - 3);
    const auto metaFileName = commonFileName + "dsc"_l1;
    const QFileInfo dumpFileInfo{dumpFileName};
    const auto crashId = dumpFileInfo.baseName();
    QStringList crashFiles;

    qCInfo(lcHockeyApp, "Uploading crash report %ls", qUtf16Printable(crashId));
    QScopedPointer<QHttpMultiPart> formData{new QHttpMultiPart{QHttpMultiPart::FormDataType}};

    {
        QFile file{metaFileName};
        if (!file.open(QFile::ReadOnly)) {
            qCWarning(lcHockeyApp, "Could not open meta information file to upload crash %ls: %ls",
                      qUtf16Printable(crashId), qUtf16Printable(file.errorString()));
            return {};
        }

        auto metaData = file.readAll();
        metaData.replace("@@CRASHID@@", crashId.toUtf8());
        metaData.replace("@@MINIDUMP_TIMESTAMP@@", dumpFileInfo.lastModified().toString(Qt::RFC2822Date).toLatin1());

        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader, s_formDataHeaderMeta.arg(crashId));
        part.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain"_l1);
        part.setBody(metaData);
        formData->append(part);

        crashFiles << file.fileName();
    }

    if (!attachFile(formData.data(), "attachment0"_l1, dumpFileName, &crashFiles, Mandatory))
        return {};

    attachFile(formData.data(), "attachment1"_l1, commonFileName + "qst"_l1, &crashFiles, Optional);
    attachFile(formData.data(), "description"_l1, commonFileName + "log"_l1, &crashFiles, Mandatory);

    QNetworkRequest request{QUrl{s_restUrlUploadCrashReport.arg(d->appId)}};

    // Qt wraps the boundary token in quotation marks and HockeyApp service doesn't like that.
    request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + formData->boundary());

    const auto reply = d->networkAccessManager()->post(request, formData.data());
    formData.take()->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [reply, crashId, crashFiles] {
        if (reply->error() == QNetworkReply::NoError) {
            for (const auto &fileName: crashFiles)
                QFile::remove(fileName);
        } else {
            qCWarning(lcHockeyApp, "Could not upload crash report %ls: %ls",
                      qUtf16Printable(crashId), qUtf16Printable(reply->errorString()));
        }

        reply->deleteLater();
    });

    return reply;
}

/*!
    \fn void HockeyAppManager::findNewVersions()

    Query HockeyApp for available new versions of the application.
*/

void HockeyAppManager::findNewVersions()
{
    if (!d || d->installedFromMarket())
        return;

    const auto appInfo = d->makeAppInfo();

    QUrlQuery query;
    query.addQueryItem("format"_l1, "json"_l1);
    query.addQueryItem("app_version"_l1, QString::number(appInfo.versionCode));
    query.addQueryItem("usage_time"_l1, QString::number(usageDuration()));
    query.addQueryItem("lang"_l1, QLocale{}.name());

    if (!appInfo.platformId.isEmpty())
        query.addQueryItem("os"_l1, appInfo.platformId);
    if (!appInfo.platformVersion.isEmpty())
        query.addQueryItem("os_version"_l1, appInfo.platformVersion);
    if (!appInfo.modelName.isEmpty())
        query.addQueryItem("device"_l1, appInfo.modelName);
    if (!appInfo.vendorName.isEmpty())
        query.addQueryItem("oem"_l1, appInfo.vendorName);
    if (!appInfo.deviceId.isEmpty())
        query.addQueryItem(d->requestParameterDeviceId(), appInfo.deviceId);

    QUrl url{s_restUrlListVersions.arg(d->appId)};
    url.setQuery(query);

    const auto reply = d->networkAccessManager()->get(QNetworkRequest{url});

    connect(reply, &QNetworkReply::finished, this, [this, reply, appInfo] {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(lcHockeyApp, "Failed to query app versions: %ls %d",
                      qUtf16Printable(reply->errorString()), reply->error());
            return;
        }

        QJsonParseError parseError;
        const auto versions = QJsonDocument::fromJson(reply->readAll(), &parseError).array();
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(lcHockeyApp, "Failed to query app versions: %ls",
                      qUtf16Printable(parseError.errorString()));
            return;
        }

        d->newVersions.clear();
        const auto actualPlatformVersion = QVersionNumber::fromString(appInfo.platformVersion);

        for (const auto &value: versions) {
            const auto entry = value.toObject();
            const auto versionCode = entry["version"_l1].toString().toInt();
            const auto largerVersionCode = versionCode > appInfo.versionCode;

            const auto packageTimestamp = qRound64(entry["timestamp"_l1].toDouble() * 1000);
            const auto installTimestamp = QFileInfo{qApp->applicationFilePath()}.lastModified().toMSecsSinceEpoch();
            const auto newerPackageFile = versionCode == appInfo.versionCode && packageTimestamp > installTimestamp;

            const auto minimumPlatformVersion = QVersionNumber::fromString(entry["minimum_os_version"_l1].toString());
            const auto requirementsMet = minimumPlatformVersion <= actualPlatformVersion;

            if ((largerVersionCode || newerPackageFile) && requirementsMet) {
                const auto version = entry["shortversion"_l1].toString();
                const auto size = qRound64(entry["appsize"_l1].toDouble());
                const auto notes = entry["notes"_l1].toString();
                const auto mandatory = entry["mandatory"_l1].toBool();

                d->newVersions += QVariant::fromValue(HockeyAppVersionInfo{version, packageTimestamp, size, notes, mandatory});
            }
        }

        if (!d->newVersions.isEmpty()) {
            std::sort(d->newVersions.begin(), d->newVersions.end(),
                      [](const QVariant &lhsVariant, const QVariant &rhsVariant) {
                const auto lhs = lhsVariant.value<HockeyAppVersionInfo>();
                const auto rhs = rhsVariant.value<HockeyAppVersionInfo>();

                if (QVersionNumber::fromString(lhs.versionName) > QVersionNumber::fromString(rhs.versionName))
                    return true;

                return lhs.timestamp > rhs.timestamp;
            });

            emit newVersionsFound(d->newVersions);
        }
    });
}

QUrl HockeyAppManager::installUrl()
{
    return QUrl{s_webUrlInstallPage.arg(d->appId)};
}

QVariantList HockeyAppManager::newVersions() const
{
    if (!d)
        return {};

    return d->newVersions;
}

QDebug operator<<(QDebug debug, const HockeyAppVersionInfo &info)
{
    return debug << "VersionInfo(version:" << info.versionName << "mandatory:" << info.mandatory << ")";
}

} // namespace KDHockeyApp
