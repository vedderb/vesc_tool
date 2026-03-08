/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

import QtQuick
import QtQuick3D

import Vedder.vesc

/*
   Pure-QML QtQuick3D replacement for the old Vesc3dItem (QQuickPaintedItem
   wrapping a QOpenGLWidget) and the deprecated Qt3D Scene3D version.

   Renders a flat textured PCB box that can be rotated via Euler angles (in
   radians) or a scalar-first quaternion, matching the original API:

       function setRotation(roll, pitch, yaw)       // radians
       function setRotationQuat(q0, q1, q2, q3)     // scalar-first quaternion
*/

View3D {
    id: root

    // ── Public API ──────────────────────────────────────────────
    // Radians → quaternion conversion identical to the old Qt3D version.
    function setRotation(roll, pitch, yaw) {
        var cr = Math.cos(-roll * 0.5)
        var sr = Math.sin(-roll * 0.5)
        var cp = Math.cos(pitch * 0.5)
        var sp = Math.sin(pitch * 0.5)
        var cy = Math.cos(-yaw * 0.5)
        var sy = Math.sin(-yaw * 0.5)

        var q0 = cr * cp * cy + sr * sp * sy
        var q1 = sr * cp * cy - cr * sp * sy
        var q2 = cr * sp * cy + sr * cp * sy
        var q3 = cr * cp * sy - sr * sp * cy

        boardNode.rotation = Qt.quaternion(q0, q1, q2, q3)
    }

    function setRotationQuat(q0, q1, q2, q3) {
        boardNode.rotation = Qt.quaternion(q0, q1, q2, q3)
    }

    // ── Environment ─────────────────────────────────────────────
    environment: SceneEnvironment {
        clearColor: Utility.getAppHexColor("normalBackground")
        backgroundMode: SceneEnvironment.Color
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    // ── Camera ──────────────────────────────────────────────────
    // Old Qt3D camera: position (0, 20, 0), upVector (0, 0, 1),
    // viewCenter (0, 0, 0), FOV 45.
    // In QtQuick3D the camera looks down -Z by default.
    // We place it on the +Y axis and rotate to look at the origin.
    PerspectiveCamera {
        id: camera
        position: Qt.vector3d(0, 200, 0)
        eulerRotation: Qt.vector3d(-90, 0, 0)
        fieldOfView: 45
        clipNear: 1
        clipFar: 10000
    }

    // ── Lighting ────────────────────────────────────────────────
    DirectionalLight {
        eulerRotation: Qt.vector3d(-45, 30, 0)
        brightness: 1.0
        ambientColor: Qt.rgba(0.5, 0.5, 0.5, 1.0)
    }

    // ── Board model ─────────────────────────────────────────────
    // Old geometry: two overlapping cuboids 4×4×1 (the body and the
    // textured top face).  In QtQuick3D the #Cube primitive is 100×100×100.
    // Scale to (40, 40, 10) to get the same 4:4:1 proportions (in cm).
    Node {
        id: boardNode

        // Body (dark gray)
        Model {
            source: "#Cube"
            scale: Qt.vector3d(0.40, 0.40, 0.10)
            materials: [
                PrincipledMaterial {
                    baseColor: "dimgray"
                }
            ]
        }

        // Top-face texture (v6plus_top.png), offset slightly on Z
        // so it sits on top of the body and doesn't z-fight.
        Model {
            source: "#Cube"
            scale: Qt.vector3d(0.399, 0.399, 0.10)
            position: Qt.vector3d(0, 0, 0.1)
            eulerRotation: Qt.vector3d(0, 0, 90)
            materials: [
                PrincipledMaterial {
                    baseColorMap: Texture {
                        source: "qrc:/res/images/v6plus_top.png"
                    }
                }
            ]
        }
    }
}
