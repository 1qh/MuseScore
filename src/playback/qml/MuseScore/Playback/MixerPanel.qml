/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents
import MuseScore.Playback

import "internal"

ColumnLayout {
    id: root

    property NavigationSection navigationSection: null
    property int contentNavigationPanelOrderStart: 1

    property alias contextMenuModel: contextMenuModel
    property Component toolbarComponent: MixerPanelToolbar {
        navigation.section: root.navigationSection
        navigation.order: root.contentNavigationPanelOrderStart
    }

    signal resizeRequested(var newWidth, var newHeight)

    spacing: 0
    implicitWidth: flickable.contentWidth + (verticalScrollBar.isScrollbarNeeded ? verticalScrollBar.implicitWidth : 0)
    implicitHeight: contentColumn.height + (horizontalScrollBar.isScrollbarNeeded ? scrollBarRow.height : 0)

    function resizePanelToContent() {
        if (contentColumn.completed && implicitHeight > 0) {
            root.resizeRequested(implicitWidth, implicitHeight)
        }
    }

    onImplicitWidthChanged: Qt.callLater(root.resizePanelToContent)
    onImplicitHeightChanged: {
        Qt.callLater(root.resizePanelToContent)
    }

    QtObject {
        id: prv

        property var currentNavigateControlIndex: undefined
        property bool isPanelActivated: false

        readonly property real headerWidth: 98
        readonly property real channelItemWidth: 108

        function setNavigateControlIndex(index) {
            if (!Boolean(prv.currentNavigateControlIndex) ||
                    index.row !== prv.currentNavigateControlIndex.row ||
                    index.column !== prv.currentNavigateControlIndex.column) {
                prv.isPanelActivated = false
            }

            prv.currentNavigateControlIndex = index
        }
    }

    function scrollToFocusedItem(focusedIndex) {
        let targetScrollPosition = (focusedIndex) * (prv.channelItemWidth + 1) // + 1 for separators
        let maxContentX = flickable.contentWidth - flickable.width

        if (targetScrollPosition + prv.channelItemWidth > flickable.contentX + flickable.width) {
            flickable.contentX = Math.min(targetScrollPosition + prv.channelItemWidth - flickable.width, maxContentX)
        } else if (targetScrollPosition < flickable.contentX) {
            flickable.contentX = Math.max(targetScrollPosition - prv.channelItemWidth, 0)
        }
    }

    MixerPanelModel {
        id: mixerPanelModel

        navigationSection: root.navigationSection
        navigationOrderStart: root.contentNavigationPanelOrderStart + 1 // +1 for toolbar

        onModelReset: {
            Qt.callLater(setupConnections)
        }

        function setupConnections() {
            for (let i = 0; i < mixerPanelModel.rowCount(); i++) {
                let item = mixerPanelModel.get(i)
                item.channelItem.panel.navigationEvent.connect(function(event) {
                    if (event.type === NavigationEvent.AboutActive) {
                        if (Boolean(prv.currentNavigateControlIndex)) {
                            event.setData("controlIndex", [prv.currentNavigateControlIndex.row, prv.currentNavigateControlIndex.column])
                            event.setData("controlOptional", true)
                        }

                        prv.isPanelActivated = true
                        scrollToFocusedItem(i)
                    }
                })
            }
        }
    }

    MixerPanelContextMenuModel {
        id: contextMenuModel

        Component.onCompleted: {
            contextMenuModel.load()
        }
    }

    StyledFlickable {
        id: flickable

        Layout.fillWidth: true
        Layout.fillHeight: true

        contentWidth: (contextMenuModel.labelsSectionVisible ? prv.headerWidth + 1 : 0)
                      + mixerPanelModel.count * (prv.channelItemWidth + 1)
        contentHeight: Math.max(contentColumn.height, height)

        implicitWidth: contentWidth
        implicitHeight: contentColumn.height

        interactive: (height < contentHeight || width < contentWidth) && !flickable.resourcePickingActive

        ScrollBar.horizontal: horizontalScrollBar

        ScrollBar.vertical: StyledScrollBar {
            id: verticalScrollBar
            policy: ScrollBar.AlwaysOn
        }

        property bool completed: false
        property bool resourcePickingActive: soundSection.resourcePickingActive

        function positionViewAtEnd() {
            if (!flickable.completed) {
                return
            }

            if (flickable.contentY == flickable.contentHeight) {
                return
            }

            flickable.contentY = flickable.contentHeight - flickable.height
        }

        onContentHeightChanged: {
            flickable.positionViewAtEnd()
        }

        Component.onCompleted: {
            flickable.completed = true
        }

        Row {
            id: separators

            anchors.fill: parent
            anchors.leftMargin: contextMenuModel.labelsSectionVisible ? prv.headerWidth : prv.channelItemWidth

            spacing: prv.channelItemWidth

            Repeater {
                model: contextMenuModel.labelsSectionVisible ? mixerPanelModel.count + 1 : mixerPanelModel.count

                SeparatorLine { orientation: Qt.Vertical }
            }
        }

        Column {
            id: contentColumn

            anchors.bottom: parent.bottom
            width: flickable.contentWidth
            spacing: 0

            property bool completed: false

            Component.onCompleted: {
                contentColumn.completed = true
                Qt.callLater(root.resizePanelToContent)
            }

            MixerSoundSection {
                id: soundSection

                visible: contextMenuModel.soundSectionVisible
                headerVisible: contextMenuModel.labelsSectionVisible
                headerWidth: prv.headerWidth
                channelItemWidth: prv.channelItemWidth
                spacingAbove: 8

                model: mixerPanelModel

                navigationRowStart: 1
                needReadChannelName: prv.isPanelActivated

                onNavigateControlIndexChanged: function(index) {
                    prv.setNavigateControlIndex(index)
                }
            }

            MixerFaderSection {
                id: faderSection

                visible: contextMenuModel.faderSectionVisible
                headerVisible: contextMenuModel.labelsSectionVisible
                headerWidth: prv.headerWidth
                channelItemWidth: prv.channelItemWidth
                spacingAbove: -3
                spacingBelow: -2

                model: mixerPanelModel

                navigationRowStart: 500
                needReadChannelName: prv.isPanelActivated

                onNavigateControlIndexChanged: function(index) {
                    prv.setNavigateControlIndex(index)
                }
            }

            MixerMuteAndSoloSection {
                id: muteAndSoloSection

                visible: contextMenuModel.muteAndSoloSectionVisible
                headerVisible: contextMenuModel.labelsSectionVisible
                headerWidth: prv.headerWidth
                channelItemWidth: prv.channelItemWidth

                model: mixerPanelModel

                navigationRowStart: 600
                needReadChannelName: prv.isPanelActivated

                onNavigateControlIndexChanged: function(index) {
                    prv.setNavigateControlIndex(index)
                }
            }

            MixerTitleSection {
                id: titleSection

                visible: contextMenuModel.titleSectionVisible
                headerVisible: contextMenuModel.labelsSectionVisible
                headerWidth: prv.headerWidth
                channelItemWidth: prv.channelItemWidth
                spacingAbove: 2
                spacingBelow: 0

                model: mixerPanelModel

                navigationRowStart: 700
                needReadChannelName: prv.isPanelActivated

                onNavigateControlIndexChanged: function(index) {
                    prv.setNavigateControlIndex(index)
                }
            }
        }
    }

    Column {
        id: scrollBarRow
        spacing: 0
        Layout.fillWidth: true
        visible: horizontalScrollBar.isScrollbarNeeded

        SeparatorLine {}

        StyledScrollBar {
            id: horizontalScrollBar
            width: parent.width
            policy: ScrollBar.AlwaysOn
        }
    }

    onHeightChanged: {
        flickable.positionViewAtEnd()
    }
}
