/*
   Copyright (C) 2017 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com.
   All rights reserved.

   This file is part of the KD HockeyApp library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of either:

     The GNU Lesser General Public License version 2.1 and version 3
     as published by the Free Software Foundation and appearing in the
     file LICENSE.LGPL.txt included.

   Or:

     The Mozilla Public License Version 2.0 as published by the Mozilla
     Foundation and appearing in the file LICENSE.MPL2.txt included.

   This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
   WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

   Contact info@kdab.com if any conditions of this licensing is not clear to you.
*/

import QtQuick 2.0

Item {
    Rectangle {
        anchors.centerIn: parent

        border {
            color: "maroon"
            width: 2
        }

        color: "silver"
        opacity: buttonArea.pressed ? 0.8 : 1

        implicitWidth: buttonLabel.implicitWidth + 8
        implicitHeight: buttonLabel.implicitHeight + 8

        Text {
            id: buttonLabel

            anchors.centerIn: parent
            color: "maroon"
            text: "Crash from QML"
        }

        MouseArea {
            id: buttonArea

            anchors.fill: parent
            onClicked: _example.crashTheAppNow()
        }
    }
}
