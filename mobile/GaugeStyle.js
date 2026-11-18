/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

.pragma library

// Colours and painting shared by the gauges and their housings. Light falls from the top left.

function palette(dark) {
    return dark ? { lo: "#1e1e22", mid: "#3a3a41", hi: "#7d7d86", hi2: "#55555d", line: "#90000000",
                    face0: "#28272c", face1: "#161519", groove: "#0b0b0d", shadow: 0.42 }
                : { lo: "#b0b0b8", mid: "#d3d3d9", hi: "#f8f8fa", hi2: "#e6e6ea", line: "#30000000",
                    face0: "#ffffff", face1: "#e8e8ec", groove: "#d2d2d8", shadow: 0.16 }
}

// Turned metal around (cx, cy), brightest at angle (radians)
function metal(ctx, cx, cy, angle, p) {
    var g = ctx.createConicalGradient(cx, cy, angle)
    g.addColorStop(0, p.hi); g.addColorStop(0.13, p.mid); g.addColorStop(0.3, p.lo)
    g.addColorStop(0.44, p.mid); g.addColorStop(0.5, p.hi2); g.addColorStop(0.56, p.mid)
    g.addColorStop(0.74, p.lo); g.addColorStop(0.88, p.mid); g.addColorStop(1, p.hi)
    return g
}

// Soft shadow under a rounded rectangle, or a circle when w = h = 2r
function shadow(ctx, x, y, w, h, r, size, p) {
    ctx.fillStyle = Qt.rgba(0, 0, 0, p.shadow / 9)
    for (var i = 6; i >= 1; i--) {
        var e = size * i / 6
        ctx.beginPath()
        ctx.roundedRect(x - e, y - e + size * 0.35, w + 2 * e, h + 2 * e, r + e, r + e)
        ctx.fill()
    }
}

// Rounded housing with a metal frame of width b (at least 2.5 px) and a recessed face
function frame(ctx, w, h, r, b, p) {
    b = Math.max(2.5, b)
    shadow(ctx, 0, 0, w, h, r, b * 1.6, p)
    var s = Math.min(w, h)
    var g = ctx.createLinearGradient(0, 0, s * 0.5, h)
    g.addColorStop(0, p.hi); g.addColorStop(0.3, p.mid); g.addColorStop(0.7, p.lo); g.addColorStop(1, p.mid)
    ctx.beginPath()
    ctx.fillStyle = g
    ctx.roundedRect(0, 0, w, h, r, r)
    ctx.fill()
    g = ctx.createLinearGradient(0, 0, s * 0.5, h)
    g.addColorStop(0, p.lo); g.addColorStop(0.6, p.lo); g.addColorStop(1, p.hi2)
    ctx.beginPath()
    ctx.fillStyle = g
    ctx.roundedRect(b * 0.55, b * 0.55, w - b * 1.1, h - b * 1.1, r - b * 0.55, r - b * 0.55)
    ctx.fill()
    ctx.beginPath()
    ctx.fillStyle = p.face1
    ctx.roundedRect(b, b, w - 2 * b, h - 2 * b, r - b, r - b)
    ctx.fill()
    // Shadow of the frame on the face
    ctx.save()
    ctx.clip()
    g = ctx.createLinearGradient(0, b, 0, b + s * 0.25)
    g.addColorStop(0, Qt.rgba(0, 0, 0, p.shadow * 0.9))
    g.addColorStop(1, "#00000000")
    ctx.fillStyle = g
    ctx.fillRect(0, b, w, s * 0.25)
    ctx.restore()
}
