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

#ifndef KDHOCKEYAPPSOFTASSERT_P_H
#define KDHOCKEYAPPSOFTASSERT_P_H

#include <QLoggingCategory>

#define KD_SOFTASSERT_FAILED(category, condition) \
    Q_UNLIKELY(!(condition) && ::KDHockeyApp::Private::softAssert \
        (QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category, #condition))

#define KD_SOFTASSERT_COMPARE_FAILED(category, actual, op, expected) \
    Q_UNLIKELY(!((actual) op (expected)) && ::KDHockeyApp::Private::softAssert \
        (QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category, \
         (actual), (expected), #actual " " #op " " #expected))

#define KD_SOFTASSERT_EQ_FAILED(category, actual, expected) \
    KD_SOFTASSERT_COMPARE_FAILED(category, actual, ==, expected)
#define KD_SOFTASSERT_GE_FAILED(category, actual, expected) \
    KD_SOFTASSERT_COMPARE_FAILED(category, actual, >=, expected)
#define KD_SOFTASSERT_LE_FAILED(category, actual, expected) \
    KD_SOFTASSERT_COMPARE_FAILED(category, actual, <=, expected)

namespace KDHockeyApp {
namespace Private {

class SoftAssertLogger
{
public:
    explicit SoftAssertLogger(const char *file, int line, const char *function,
                              QMessageLogger::CategoryFunction categoryFunction);
    ~SoftAssertLogger();

    template<typename T>
    const SoftAssertLogger &operator<<(const T &value) const
    {
        if (m_debugStream)
            *m_debugStream << value;

        return *this;
    }

private:
    QDebug *const m_debugStream;
};

Q_REQUIRED_RESULT bool softAssert(const char *file, int line, const char *function,
                                  QMessageLogger::CategoryFunction categoryFunction,
                                  const char *expression);

template<typename T, typename U>
Q_REQUIRED_RESULT inline bool softAssert(const char *file, int line, const char *function,
                                         QMessageLogger::CategoryFunction categoryFunction,
                                         const T &actualValue, const U &expectedValue,
                                         const char *expression)
{
    SoftAssertLogger{file, line, function, categoryFunction}
            << "Assertion failed" << function << ": " << expression
            << " (actual value: " << actualValue << ", expected value: " << expectedValue << ")";
    return true;
}

} // namespace Private
} // namespace KDHockeyApp

#endif // KDHOCKEYAPPSOFTASSERT_P_H
