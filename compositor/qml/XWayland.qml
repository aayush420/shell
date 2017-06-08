/****************************************************************************
 * This file is part of Liri.
 *
 * Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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
import QtWayland.Compositor 1.0
import Liri.XWayland 1.0 as LXW
import "windows"

LXW.XWayland {
    id: xwayland

    enabled: true
    manager: LXW.XWaylandManager {
        id: manager
        onShellSurfaceRequested: {
            var shellSurface = shellSurfaceComponent.createObject(manager);
            shellSurface.initialize(manager, window, geometry, overrideRedirect, parentShellSurface);
        }
        onShellSurfaceCreated: __private.handleShellSurfaceCreated(shellSurface, xchromeComponent)
    }

    Component {
        id: shellSurfaceComponent

        LXW.XWaylandShellSurface {
            id: shellSurface

            property bool minimized: false
            property string canonicalAppId
            property string iconName: "unknown"

            readonly property bool decorated: windowType != Qt.Popup && decorate
            readonly property bool hasDropShadow: !maximized && !fullscreen

            property WaylandSurface parentWlSurface: parentSurface ? parentSurface.surface : null
            property point offset: Qt.point(0, 0)

            readonly property alias moveItem: moveItem

            QtObject {
                id: details

                property bool registered: false
                property bool responsive: true
            }

            MoveItem {
                id: moveItem

                parent: rootItem
                width: surface ? surface.width : 0
                height: surface ? surface.height : 0

                onXChanged: if (surface) shellSurface.sendX(x)
                onYChanged: if (surface) shellSurface.sendY(y)
            }

            onSurfaceChanged: {
                canonicalAppId = applicationManager.canonicalizeAppId(appId);
            }
            onActivatedChanged: {
                if (details.registered && activated && windowType != Qt.Popup)
                    applicationManager.focusShellSurface(shellSurface);
            }
            onCanonicalAppIdChanged: {
                if (!details.registered && canonicalAppId && windowType != Qt.Popup) {
                    // Register application
                    applicationManager.registerShellSurface(shellSurface);
                    details.registered = true;

                    // Focus icon in the panel
                    if (activated)
                        applicationManager.focusShellSurface(shellSurface);
                }

                // Set icon name when appId changes
                var appIconName = applicationManager.getIconName(canonicalAppId);
                iconName = appIconName ? appIconName : "unknown";
            }
            onWindowTypeChanged: {
                switch (windowType) {
                case Qt.SubWindow:
                case Qt.Popup:
                    offset.x = Qt.binding(function() { return x; });
                    offset.y = Qt.binding(function() { return y; });
                    break;
                default:
                    offset.x = 0;
                    offset.y = 0;
                    break;
                }
            }

            function requestSize(w, h) {
                shellSurface.sendResize(Qt.size(w, h));
            }

            Component.onDestruction: {
                __private.handleShellSurfaceDestroyed(shellSurface);
            }
        }
    }

    Component {
        id: xchromeComponent

        XWaylandChrome {
            inputEventsEnabled: !output.screenView.locked
        }
    }
}
