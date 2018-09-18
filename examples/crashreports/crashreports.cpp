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

#include <KDHockeyAppConfig.h>
#include <KDHockeyAppManager.h>

#include <QApplication>
#include <QBoxLayout>
#include <QMainWindow>
#include <QPushButton>

#ifdef KDHOCKEYAPP_QMLSUPPORT_ENABLED
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWidget>
#endif // KDHOCKEYAPP_QMLSUPPORT_ENABLED

#include <functional>

namespace KDHockeyAppManager {
namespace Examples {

class CrashReports : public QApplication
{
    Q_OBJECT

public:
    using QApplication::QApplication;

    int run()
    {
        // Crash reports are written to the application's cache folder.
        // Setting the organization and application make sure that this folder
        // is created at a reasonable place.
        setOrganizationName(QStringLiteral("KDAB"));

        // The HockeyApp id identifies the application in your HockeyApp console.
        KDHockeyApp::HockeyAppManager hockeyApp{"4835ed47abf14aba8ed1480a341450f2"};

        // We should upload crash reports as soon as possible to ensure even
        // crashes in initialization code are reported.
        hockeyApp.uploadCrashDumps();

#ifdef KDHOCKEYAPP_QMLSUPPORT_ENABLED
        // We also create the QML engine early to ensure to receive QML
        // backtraces for crashes while loading QML files.
        QQmlEngine qml;
        qml.rootContext()->setContextProperty("_example", this);
        hockeyApp.setQmlEngine(&qml);
#endif // KDHOCKEYAPP_QMLSUPPORT_ENABLED

        // Now only some widgets need to be setup and shown.
        // Nothing HockeyApp specific for the rest of this method.
        QMainWindow window;
        window.setCentralWidget(new QWidget{&window});
        QBoxLayout layout{QBoxLayout::TopToBottom, window.centralWidget()};

        QPushButton crashFromCxx{tr("Crash from C++"), window.centralWidget()};
        connect(&crashFromCxx, &QPushButton::clicked, this, &CrashReports::crashTheAppNow);
        layout.addWidget(&crashFromCxx);

#ifdef KDHOCKEYAPP_QMLSUPPORT_ENABLED
        QQuickWidget crashFromQml{&qml, window.centralWidget()};
        crashFromQml.setResizeMode(QQuickWidget::SizeRootObjectToView);
        crashFromQml.setSource(QUrl{"qrc:/crashreports.qml"});
        layout.addWidget(&crashFromQml);
#endif // KDHOCKEYAPP_QMLSUPPORT_ENABLED

        window.show();

        return exec();
    }

public slots:
    void crashTheAppNow()
    {
        // This code provides a segfault by dereferencing a null pointer
        // and writing to that read-only memory location.
        std::function<void(int)> crashForTesting = [&](int n) {
            if (0 >= n) {
                qInfo("provoking segfault for testing purposes");
                char *const crashingCharPointer = nullptr;
                crashingCharPointer[0] = 'X';
            }

            crashForTesting(n - 1);
        };

        // Let's randomize the backtraces a bit.
        qsrand(static_cast<uint>(time({})));
        crashForTesting(rand() % 20);
    }
};

} // namespace KDHockeyAppManager
} // namespace Examples

int main(int argc, char *argv[])
{
    return  KDHockeyAppManager::Examples::CrashReports{argc, argv}.run();
}

#include "crashreports.moc"
