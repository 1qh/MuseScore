/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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

import Muse.Ui
import Muse.UiComponents
import Muse.Dock
import Muse.Extensions
import MuseScore.AppShell

import MuseScore.NotationScene
import MuseScore.Palette
import MuseScore.Inspector
import MuseScore.InstrumentsScene
import MuseScore.Playback

DockPage {
    id: root

    objectName: "Notation"
    uri: "musescore://notation"

    required property NavigationSection topToolbarKeyNavSec

    property NotationPageModel pageModel: NotationPageModel {}

    property NavigationSection noteInputKeyNavSec: NavigationSection {
        name: "NoteInputSection"
        order: 2
    }

    property NavigationSection keynavTopPanelSec: NavigationSection {
        name: "NavigationTopPanel"
        enabled: root.visible
        order: 3
    }

    property NavigationSection keynavLeftPanelSec: NavigationSection {
        name: "NavigationLeftPanel"
        enabled: root.visible
        order: 4
    }

    property NavigationSection keynavRightPanelSec: NavigationSection {
        name: "NavigationRightPanel"
        enabled: root.visible
        order: 6
    }

    property NavigationSection keynavBottomPanelSec: NavigationSection {
        name: "NavigationBottomPanel"
        enabled: root.visible
        order: 7
    }

    function navigationPanelSec(location) {
        switch(location) {
        case Location.Top: return keynavTopPanelSec
        case Location.Left: return keynavLeftPanelSec
        case Location.Right: return keynavRightPanelSec
        case Location.Bottom: return keynavBottomPanelSec
        }

        return null
    }

    onInited: {
        Qt.callLater(pageModel.init)
    }

    readonly property int verticalPanelDefaultWidth: 300

    readonly property int horizontalPanelMinHeight: 100
    readonly property int horizontalPanelMaxHeight: 520

    readonly property int panelMinDimension: 10
    readonly property int panelMaxDimension: 7500 //! NOTE: Value found experimentally - see issue #27770

    readonly property string verticalPanelsGroup: "VERTICAL_PANELS"
    readonly property string horizontalPanelsGroup: "HORIZONTAL_PANELS"

    readonly property var verticalPanelDropDestinations: [
        { "dock": root.centralDock, "dropLocation": Location.Left, "dropDistance": root.verticalPanelDefaultWidth },
        { "dock": root.centralDock, "dropLocation": Location.Right, "dropDistance": root.verticalPanelDefaultWidth }
    ]

    readonly property var horizontalPanelDropDestinations: [
        root.panelTopDropDestination,
        root.panelBottomDropDestination
    ]

    property var notationView: null

    mainToolBars: [
        DockToolBar {
            id: notationToolBar

            defaultFloating: true

            objectName: "notationToolBar"

            // Hidden on open: the score view starts as the sheet alone
            visible: false
            title: qsTrc("appshell", "Notation toolbar")

            floatable: true
            closable: true
            resizable: false
            separatorsVisible: false

            alignment: DockToolBarAlignment.Center
            contentBottomPadding: 2

            navigationSection: root.topToolbarKeyNavSec

            NotationToolBar {
                navigationPanel.section: notationToolBar.navigationSection
                navigationPanel.order: 2
            }
        },

        DockToolBar {
            id: playbackToolBar

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.playbackToolBarName()

            // Hidden on open: the score view starts as the sheet alone
            visible: false
            title: qsTrc("appshell", "Playback controls")

            separatorsVisible: false
            alignment: DockToolBarAlignment.Right
            resizable: !floating

            contentBottomPadding: floating ? 8 : 2
            contentTopPadding: floating ? 8 : 0

            dropDestinations: [
                { "dock": notationToolBar, "dropLocation": Location.Right }
            ]

            navigationSection: root.topToolbarKeyNavSec

            PlaybackToolBar {
                navigationPanelSection: playbackToolBar.navigationSection
                navigationPanelOrder: 3

                floating: playbackToolBar.floating
            }
        },

        DockToolBar {
            id: extDockToolBar

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.extensionsToolBarName()

            // Hidden on open: the score view starts as the sheet alone
            visible: false
            title: qsTrc("appshell", "Extensions toolbar")

            resizable: !floating
            separatorsVisible: false
            orientation: Qt.Horizontal
            alignment: DockToolBarAlignment.Right

            contentBottomPadding: floating ? 8 : 2
            contentTopPadding: floating ? 8 : 0

            dropDestinations: [
                { "dock": notationToolBar, "dropLocation": Location.Right },
                { "dock": playbackToolBar, "dropLocation": Location.Right }
            ]

            navigationSection: root.topToolbarKeyNavSec

            ExtensionsToolBar {
                id: extToolBar

                navigationPanel.section: extDockToolBar.navigationSection
                navigationPanel.order: 4
            }
        },

        DockToolBar {
            id: undoRedoToolBar

            defaultFloating: true

            objectName: root.pageModel.undoRedoToolBarName()

            // Hidden on open: the score view starts as the sheet alone
            visible: false
            title: qsTrc("appshell", "Undo/redo")

            floatable: true
            closable: true
            resizable: false
            separatorsVisible: false

            alignment: DockToolBarAlignment.Right
            contentBottomPadding: 2

            navigationSection: root.topToolbarKeyNavSec

            UndoRedoToolBar {
                navigationPanel.section: undoRedoToolBar.navigationSection
                navigationPanel.order: 5
            }
        }
    ]

    toolBars: [
        DockToolBar {
            id: noteInputBar

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.noteInputBarName()

            // Hidden on open: the score view starts as the sheet alone
            visible: false
            title: qsTrc("appshell", "Note input")

            dropDestinations: [
                root.toolBarTopDropDestination,
                root.toolBarBottomDropDestination,
                root.toolBarLeftDropDestination,
                root.toolBarRightDropDestination
            ]

            thickness: orientation === Qt.Horizontal ? 40 : 76
            resizable: !floating

            navigationSection: root.noteInputKeyNavSec

            NoteInputBar {
                orientation: noteInputBar.orientation
                floating: noteInputBar.floating

                maximumWidth: noteInputBar.width
                maximumHeight: noteInputBar.height

                navigationPanel.section: noteInputBar.navigationSection
                navigationPanel.order: 1
            }
        }
    ]

    panels: [
        DockPanel {
            id: palettesPanel
            fitFloatingContent: true

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.palettesPanelName()

            // Hidden on open: the score view starts as the sheet alone
            visible: false
            title: qsTrc("appshell", "Palettes")

            navigationSection: root.navigationPanelSec(palettesPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: floating ? root.panelMinDimension : root.verticalPanelDefaultWidth
            maximumWidth: floating ? Math.max(root.panelMaxDimension, naturalContentSize.width) : root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: Math.max(root.panelMaxDimension, naturalContentSize.height)

            groupName: root.verticalPanelsGroup

            dropDestinations: root.verticalPanelDropDestinations

            PalettesPanel {
                navigationSection: palettesPanel.navigationSection
                navigationOrderStart: palettesPanel.contentNavigationPanelOrderStart

                Component.onCompleted: {
                    palettesPanel.contextMenuModel = contextMenuModel
                }
            }
        },

        DockPanel {
            id: layoutPanel
            fitFloatingContent: true

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.layoutPanelName()

            // Hidden on open: the score view starts as the sheet alone
            visible: false
            title: qsTrc("appshell", "Layout")

            navigationSection: root.navigationPanelSec(layoutPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: floating ? root.panelMinDimension : root.verticalPanelDefaultWidth
            maximumWidth: floating ? Math.max(root.panelMaxDimension, naturalContentSize.width) : root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: Math.max(root.panelMaxDimension, naturalContentSize.height)

            groupName: root.verticalPanelsGroup

            dropDestinations: root.verticalPanelDropDestinations

            LayoutPanel {
                navigationSection: layoutPanel.navigationSection
                navigationOrderStart: layoutPanel.contentNavigationPanelOrderStart

                Component.onCompleted: {
                    layoutPanel.contextMenuModel = contextMenuModel
                }
            }
        },

        DockPanel {
            id: inspectorPanel

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.inspectorPanelName()

            // Hidden on open: the score view starts as the sheet alone
            visible: false
            title: qsTrc("appshell", "Properties")

            navigationSection: root.navigationPanelSec(inspectorPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: floating ? root.panelMaxDimension : root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            groupName: root.verticalPanelsGroup

            dropDestinations: root.verticalPanelDropDestinations

            InspectorForm {
                navigationSection: inspectorPanel.navigationSection
                navigationOrderStart: inspectorPanel.contentNavigationPanelOrderStart
                notationView: root.notationView
            }
        },

        DockPanel {
            id: selectionFilterPanel
            fitFloatingContent: true

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.selectionFiltersPanelName()
            title: qsTrc("appshell", "Selection filter")

            navigationSection: root.navigationPanelSec(selectionFilterPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: floating ? root.panelMinDimension : root.verticalPanelDefaultWidth
            maximumWidth: floating ? Math.max(root.panelMaxDimension, naturalContentSize.width) : root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: Math.max(root.panelMaxDimension, naturalContentSize.height)

            groupName: root.verticalPanelsGroup

            //! NOTE: hidden by default
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            SelectionFilterPanel {
                navigationSection: selectionFilterPanel.navigationSection
                navigationOrderStart: selectionFilterPanel.contentNavigationPanelOrderStart
            }
        },

        DockPanel {
            id: undoHistoryPanel
            fitFloatingContent: true

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.undoHistoryPanelName()
            title: qsTrc("notation", "History")

            navigationSection: root.navigationPanelSec(undoHistoryPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: floating ? root.panelMinDimension : root.verticalPanelDefaultWidth
            maximumWidth: floating ? Math.max(root.panelMaxDimension, naturalContentSize.width) : root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: Math.max(root.panelMaxDimension, naturalContentSize.height)

            groupName: root.verticalPanelsGroup
            location: Location.Right

            //! NOTE: hidden by default
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            UndoHistoryPanel {
                navigationSection: undoHistoryPanel.navigationSection
                navigationOrderStart: undoHistoryPanel.contentNavigationPanelOrderStart
            }
        },
        
        // =============================================
        // Horizontal Panels
        // =============================================

        DockPanel {
            id: mixerPanel

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.mixerPanelName()
            title: qsTrc("appshell", "Mixer")

            height: 368
            minimumHeight: root.horizontalPanelMinHeight
            maximumHeight: floating ? root.panelMaxDimension : root.horizontalPanelMaxHeight

            minimumWidth: root.panelMinDimension
            maximumWidth: root.panelMaxDimension

            groupName: root.horizontalPanelsGroup

            //! NOTE: hidden by default
            visible: false

            location: Location.Bottom

            dropDestinations: root.horizontalPanelDropDestinations

            navigationSection: root.navigationPanelSec(mixerPanel.location)

            MixerPanel {
                id: mixerPanelComponent

                navigationSection: mixerPanel.navigationSection
                contentNavigationPanelOrderStart: mixerPanel.contentNavigationPanelOrderStart

                Component.onCompleted: {
                    mixerPanel.contextMenuModel = contextMenuModel
                    mixerPanel.toolbarComponent = toolbarComponent
                }

                Component.onDestruction: {
                    mixerPanel.contextMenuModel = null
                    mixerPanel.toolbarComponent = null
                }

                onResizeRequested: function(newWidth, newHeight) {
                    mixerPanel.resize(mixerPanel.floating ? newWidth : mixerPanel.width, newHeight)
                }

                Connections {
                    target: mixerPanel
                    function onPanelShown() {
                        Qt.callLater(mixerPanelComponent.resizePanelToContent)
                    }
                }
            }
        },

        DockPanel {
            id: pianoKeyboardPanel

            objectName: root.pageModel.pianoKeyboardPanelName()
            title: qsTrc("appshell", "Piano keyboard")

            height: 200
            minimumHeight: root.horizontalPanelMinHeight
            maximumHeight: root.horizontalPanelMaxHeight

            minimumWidth: root.panelMinDimension
            maximumWidth: root.panelMaxDimension

            groupName: "pianoKeyboard"

            visible: false

            location: Location.Top

            dropDestinations: root.horizontalPanelDropDestinations

            navigationSection: root.navigationPanelSec(pianoKeyboardPanel.location)

            PianoKeyboardPanel {
                id: pianoKeyboardContent

                navigationSection: pianoKeyboardPanel.navigationSection
                contentNavigationPanelOrderStart: pianoKeyboardPanel.contentNavigationPanelOrderStart

                Component.onCompleted: {
                    pianoKeyboardPanel.contextMenuModel = contextMenuModel
                }

                function fitPanel() {
                    if (fittedHeight > 0) {
                        pianoKeyboardPanel.resize(pianoKeyboardPanel.width, fittedHeight)
                    }
                }

                onFittedHeightChanged: fitPanel()

                Connections {
                    target: pianoKeyboardPanel
                    function onPanelShown() {
                        pianoKeyboardContent.fitPanel()
                    }
                }
            }
        },

        DockPanel {
            id: navigatorPanel
            defaultFloating: true
            fitFloatingContent: true
            floatable: true
            objectName: root.pageModel.navigatorPanelName()
            title: qsTrc('appshell', 'Navigator')
            width: root.verticalPanelDefaultWidth
            height: root.horizontalPanelMinHeight
            visible: false
            minimumWidth: root.panelMinDimension
            minimumHeight: root.panelMinDimension
            maximumWidth: Math.max(root.panelMaxDimension, naturalContentSize.width)
            maximumHeight: Math.max(root.panelMaxDimension, naturalContentSize.height)
            location: Location.Bottom
            navigationSection: root.navigationPanelSec(navigatorPanel.location)
            Item {
                implicitWidth: navigator.implicitWidth
                implicitHeight: navigator.implicitHeight
                clip: true
                NotationNavigator {
                    id: navigator
                    anchors.fill: parent
                    objectName: 'MainNotationViewNavigator'
                    fitFullContent: true
                    // root.notationView is the score's paint view itself, and may still be null when this panel loads
                    readonly property rect sourceViewport: root.notationView ? root.notationView.viewport : Qt.rect(0, 0, 0, 0)
                    onSourceViewportChanged: setCursorRect(sourceViewport)
                    Component.onCompleted: {
                        load()
                        setCursorRect(sourceViewport)
                    }
                    onMoveNotationRequested: function(dx, dy) {
                        if (root.notationView) {
                            root.notationView.moveCanvas(dx, dy)
                        }
                    }
                }
            }
        },
        DockPanel {
            id: timelinePanel
            fitFloatingContent: true

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.timelinePanelName()
            title: qsTrc("appshell", "Timeline")

            height: 200
            minimumHeight: floating ? root.panelMinDimension : root.horizontalPanelMinHeight
            maximumHeight: floating ? Math.max(root.panelMaxDimension, naturalContentSize.height) : root.horizontalPanelMaxHeight

            minimumWidth: root.panelMinDimension
            maximumWidth: Math.max(root.panelMaxDimension, naturalContentSize.width)

            groupName: root.horizontalPanelsGroup

            //! NOTE: hidden by default
            visible: false

            location: Location.Bottom

            dropDestinations: root.horizontalPanelDropDestinations

            navigationSection: root.navigationPanelSec(timelinePanel.location)

            Timeline {
                navigationSection: timelinePanel.navigationSection
                contentNavigationPanelOrderStart: timelinePanel.contentNavigationPanelOrderStart
            }
        },

        DockPanel {
            id: drumsetPanel
            fitFloatingContent: true

            defaultFloating: true

            objectName: root.pageModel.drumsetPanelName()
            title: qsTrc("appshell", "Drumset tools")

            minimumWidth: root.panelMinDimension
            maximumWidth: Math.max(root.panelMaxDimension, naturalContentSize.width)

            height: 64
            minimumHeight: floating ? root.panelMinDimension : 64
            maximumHeight: floating ? Math.max(root.panelMaxDimension, naturalContentSize.height) : 64

            //! NOTE: hidden by default
            visible: false

            floatable: true
            closable: false

            location: Location.Bottom

            navigationSection: root.navigationPanelSec(drumsetPanel.location)

            DrumsetPanel {
                navigationSection: drumsetPanel.navigationSection
                contentNavigationPanelOrderStart: drumsetPanel.contentNavigationPanelOrderStart
            }
        },

        DockPanel {
            id: percussionPanel
            fitFloatingContent: true

            defaultFloating: true
            floatable: true

            objectName: root.pageModel.percussionPanelName()
            title: qsTrc("appshell", "Percussion")

            height: 200
            minimumHeight: floating ? root.panelMinDimension : root.horizontalPanelMinHeight
            maximumHeight: floating ? Math.max(root.panelMaxDimension, naturalContentSize.height) : root.horizontalPanelMaxHeight

            minimumWidth: root.panelMinDimension
            maximumWidth: Math.max(root.panelMaxDimension, naturalContentSize.width)

            groupName: root.horizontalPanelsGroup

            //! NOTE: hidden by default
            visible: false

            location: Location.Bottom

            dropDestinations: root.horizontalPanelDropDestinations

            navigationSection: root.navigationPanelSec(percussionPanel.location)

            PercussionPanel {
                id: percussionComponent

                navigationSection: percussionPanel.navigationSection
                contentNavigationPanelOrderStart: percussionPanel.contentNavigationPanelOrderStart

                Component.onCompleted: {
                    percussionPanel.toolbarComponent = toolbarComponent
                }

                Component.onDestruction: {
                    percussionPanel.toolbarComponent = null
                }

                onResizeRequested: function(newWidth, newHeight) {
                    if (!percussionPanel.floating) {
                        percussionPanel.resize(newWidth, newHeight)
                    }
                }

                Connections {
                    target: percussionPanel
                    function onPanelShown() {
                        if (!percussionPanel.floating) {
                            percussionComponent.resizePanelToContentHeight()
                        }
                    }
                }
            }
        }
    ]

    central: NotationView {
        id: notationView
        name: "MainNotationView"

        isNavigatorVisible: false
        isBraillePanelVisible: root.pageModel.isBraillePanelVisible
        isMainView: true

        Component.onCompleted: {
            root.notationView = notationView.paintView

            root.setDefaultNavigationControl(notationView.defaultNavigationControl)
        }

        Component.onDestruction: {
            root.setDefaultNavigationControl(null)
        }
    }

    statusBar: DockStatusBar {
        objectName: root.pageModel.statusBarName()

        visible: false

        navigationSection: content.navigationSection

        NotationStatusBar {
            id: content
        }
    }
}
