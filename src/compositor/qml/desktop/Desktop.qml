/****************************************************************************
 * This file is part of Liri.
 *
 * Copyright (C) 2018 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 * Copyright (C) 2017 Michael Spencer <sonrisesoftware@gmail.com>
 *
 * $BEGIN_LICENSE:GPL3+$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $END_LICENSE$
 ***************************************************************************/

import QtQuick 2.0
import QtWayland.Compositor 1.0 as QtWaylandCompositor
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import Fluid.Controls 1.0 as FluidControls
import Liri.Shell 1.0
import ".."
import "../components"
import "../indicators"
import "../notifications"

Item {
    id: desktop

    Material.theme: Material.Dark
    Material.primary: Material.Blue
    Material.accent: Material.Blue

    readonly property var layers: QtObject {
        readonly property alias workspaces: workspace //workspacesLayer
        readonly property alias fullScreen: fullScreenLayer
        readonly property alias overlays: overlaysLayer
    }

    readonly property alias shell: shellLoader.item
    readonly property alias currentWorkspace: workspace
    readonly property var panel: shell ? shell.panel : null
    readonly property alias windowSwitcher: windowSwitcher

    /*
     * Workspace
     */

    Background {
        id: backgroundLayer
        anchors.fill: parent
        mode: liriCompositor.settings.background.mode
        pictureUrl: liriCompositor.settings.background.pictureUrl
        primaryColor: liriCompositor.settings.background.primaryColor
        secondaryColor: liriCompositor.settings.background.secondaryColor
        fillMode: liriCompositor.settings.background.fillMode
        blur: false
        z: 0
    }

    DesktopWidgets {
        id: desktopLayer
        anchors.fill: parent
        z: 1
    }

    // Surface item for shell helper's grab surface
    QtWaylandCompositor.WaylandQuickItem {
        surface: shellHelper.grabSurface
        focusOnClick: false
        onSurfaceChanged: shellHelper.grabCursor(ShellHelper.ArrowGrabCursor)
    }

    // Change pointer cursor when needed
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        onEntered: shellHelper.grabCursor(ShellHelper.ArrowGrabCursor)
        onExited: shellHelper.grabCursor(ShellHelper.ArrowGrabCursor)
    }

    // Workspaces
    /*
    WorkspacesView {
        id: workspacesLayer
        anchors.fill: parent
        z: 5
    }
    */

    // FIXME: Temporary workaround to make keyboard input work,
    // apparently SwipeView captures input. An Item instead make it work.
    Workspace {
        id: workspace
        anchors.fill: parent
        onEffectStarted: if (effect == "present") parent.state = "present"
        onEffectStopped: if (effect == "present") parent.state = "normal"
        z: 6
    }

    // Panels
    Loader {
        id: shellLoader
        anchors.fill: parent
        active: output.primary
        sourceComponent: Shell {
            opacity: workspace.state == "present" ? 0.0 : 1.0
            visible: opacity > 0.0

            Behavior on opacity {
                NumberAnimation {
                    easing.type: Easing.OutQuad
                    duration: 250
                }
            }
        }
        z: 10
    }

    // Full screen windows can cover application windows and panels
    Rectangle {
        id: fullScreenLayer
        anchors.fill: parent
        color: "black"
        z: 20
        opacity: children.length > 0 ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation {
                easing.type: Easing.InSine
                duration: FluidControls.Units.mediumDuration
            }
        }
    }

    // Overlays are above the panel
    Overlay {
        id: overlaysLayer
        anchors.centerIn: parent
        z: 10
    }

    // Notifications are behind the panel
    Loader {
        id: notificationsLayer
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
            topMargin: FluidControls.Units.largeSpacing * 3
            bottomMargin: 56 + FluidControls.Units.smallSpacing
        }
        active: output.primary
        sourceComponent: Notifications {}
        width: FluidControls.Units.gu(24) + (2 * FluidControls.Units.smallSpacing)
        z: 10
    }

    // Windows switcher
    WindowSwitcher {
        id: windowSwitcher
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
    }

    /*
     * Miscellaneaous
     */

    Connections {
        target: volumeControl
        onVolumeChanged: {
            // Show overlay
            overlaysLayer.iconName = volumeControl.getIconName()
            overlaysLayer.value = volume
            overlaysLayer.showProgress = true
            if (!overlaysLayer.visible)
                overlaysLayer.show()
        }
        onMutedChanged: {
            // Show overlay
            overlaysLayer.iconName = volumeControl.getIconName()
            overlaysLayer.showProgress = false
            if (!overlaysLayer.visible)
                overlaysLayer.show()
        }
    }
}
