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

#ifndef KDHOCKEYAPPLITERALS_P_H
#define KDHOCKEYAPPLITERALS_P_H

#include <QLatin1String>
#include <QRegularExpression>
#include <QUrl>

namespace KDHockeyApp {
namespace Literals {

/**
 * The _l1 literal produces a QLatin1String.
 *
 * This is most useful to use fast-past optimizations during string comparison and similar.
 */
inline constexpr QLatin1String operator"" _l1 (const char *str, std::size_t len)
{
    return QLatin1String(str, static_cast<int>(len));
}

inline constexpr QChar operator"" _l1 (char ch)
{
    return QChar::fromLatin1(ch);
}

} // namespace Literals
} // namespace KDHockeyApp

#ifndef KDHOCKEYAPP_DISABLE_USERLITERALS
using namespace KDHockeyApp::Literals;
#endif // KDHOCKEYAPP_DISABLE_USERLITERALS

#endif // KDHOCKEYAPPLITERALS_P_H
