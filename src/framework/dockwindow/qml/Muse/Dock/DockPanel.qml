/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
import QtQuick

import Muse.Dock

DockPanelView {
    id: root

    default property alias contentComponent : contentLoader.sourceComponent
    property bool fitFloatingContent: false
    readonly property size naturalContentSize: Qt.size(contentLoader.item?.implicitWidth ?? 0,
                                                       contentLoader.item?.implicitHeight ?? 0)
    function fitContent() {
        if (!fitFloatingContent || !floating || !visible || !contentLoader.item) {
            return
        }
        const content = contentLoader.item
        if (content.implicitWidth > 0 && content.implicitHeight > 0) {
            resize(Math.ceil(content.implicitWidth), Math.ceil(content.implicitHeight))
        }
    }
    onPanelShown: Qt.callLater(root.fitContent)
    onFloatingChanged: Qt.callLater(root.fitContent)
    Connections {
        target: contentLoader.item
        function onImplicitWidthChanged() { Qt.callLater(root.fitContent) }
        function onImplicitHeightChanged() { Qt.callLater(root.fitContent) }
    }

    navigationSection: Boolean(contentLoader.item) && Boolean(contentLoader.item.navigationSection) ?
                           contentLoader.item.navigationSection : null

    Loader {
        id: contentLoader

        active: root.visible && root.inited
        onLoaded: Qt.callLater(root.fitContent)
    }
}
