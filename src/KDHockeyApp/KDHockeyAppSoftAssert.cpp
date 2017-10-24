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

#include "KDHockeyAppSoftAssert_p.h"

namespace KDHockeyApp {
namespace Private {

namespace {

Q_LOGGING_CATEGORY(lcSoftAssert, "kdab.softassert")

QDebug *debugStream(const char *file, int line, const char *function,
                    QMessageLogger::CategoryFunction categoryFunction)
{
    const auto &category = categoryFunction ? categoryFunction() : lcSoftAssert();

    if (category.isCriticalEnabled()) {
        const QMessageLogger logger{file, line, function, category.categoryName()};
        return new QDebug{logger.critical().noquote()};
    }

    return {};
}

} // namespace

SoftAssertLogger::SoftAssertLogger(const char *file, int line, const char *function, QMessageLogger::CategoryFunction categoryFunction)
    : m_debugStream(debugStream(file, line, function, categoryFunction))
{
}

SoftAssertLogger::~SoftAssertLogger()
{
    delete m_debugStream;

    if (qEnvironmentVariableIsSet("QT_QTESTLIB_RUNNING"))
        qFatal("Aborting this test suite.");
}

bool softAssert(const char *file, int line, const char *function,
                QMessageLogger::CategoryFunction categoryFunction,
                const char *expression)
{
    SoftAssertLogger{file, line, function, categoryFunction}
            << "Assertion failed in" << function << ": " << expression;
    return true;
}

} // namespace Private
} // namespace KDHockeyApp
