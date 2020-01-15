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

import Qt3D.Core 2.0
import Qt3D.Render 2.9
import Qt3D.Input 2.0
import Qt3D.Extras 2.9
import QtQuick.Scene3D 2.0

Scene3D {
    id: scene
    cameraAspectRatioMode: Scene3D.AutomaticAspectRatio
    focus: true
    multisample: true
    property alias fielfOfView: camera.fieldOfView

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

        vescObj.tr.rotation = Qt.quaternion(q0, q1, q2, q3)
    }

    function setRotationQuat(q0, q1, q2, q3) {
        vescObj.tr.rotation = Qt.quaternion(q0, q1, q2, q3)
    }

    Entity {
        id: sceneRoot

        Camera {
            id: camera
            projectionType: CameraLens.PerspectiveProjection
            fieldOfView: 45
            nearPlane : 0.1
            farPlane : 1000.0
            position: Qt.vector3d( 0.0, 20.0, 0.0 )
            upVector: Qt.vector3d( 0.0, 0.0, 1.0 )
            viewCenter: Qt.vector3d( 0.0, 0.0, 0.0 )
        }

        components: [
            RenderSettings {
                activeFrameGraph: ForwardRenderer {
                    camera: camera
                    clearColor: "transparent"
                }
            }
        ]

        Entity {
            id: vescObj

            Entity {
                id: obj1
                components: [
                    CuboidMesh {
                        xExtent: 4
                        yExtent: 4
                        zExtent: 1
                    },

                    PhongMaterial {
                        diffuse: "darkgray"
                        ambient: "dimgray"
                    }
                ]
            }

            Entity {
                id: obj2
                components: [
                    CuboidMesh {
                        id: mesh2
                        xExtent: 3.99
                        yExtent: 3.99
                        zExtent: 1.0
                    },

                    TextureMaterial {
                        texture: Texture2D {
                            TextureImage {
                                source: "qrc:/res/images/v6plus_top.png"
                            }
                        }
                    },

                    Transform {
                        translation: Qt.vector3d(0.0, 0.0, 0.01)
                        rotationZ: 90
                    }
                ]
            }

            property Transform tr: Transform {}

            components: [obj1, obj2, tr]
        }
    }
}
