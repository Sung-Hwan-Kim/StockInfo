import QtQuick
import QtQuick.Controls

// 글꼴에 의존하지 않도록 아이콘을 직접 그리는 툴바 버튼
// glyph: "back" / "menu" / "refresh"
ToolButton {
    id: btn
    property string glyph: "menu"
    property bool spinning: false
    property color iconColor: App.colors.text

    implicitWidth: 48
    implicitHeight: 48

    contentItem: Item {
        Canvas {
            id: canvas
            anchors.centerIn: parent
            width: 24
            height: 24
            antialiasing: true

            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = btn.iconColor
                ctx.fillStyle = btn.iconColor
                ctx.lineWidth = 2.2
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                if (btn.glyph === "back") {
                    ctx.beginPath()
                    ctx.moveTo(15, 5); ctx.lineTo(8, 12); ctx.lineTo(15, 19)
                    ctx.stroke()
                } else if (btn.glyph === "menu") {
                    for (let i = 0; i < 3; ++i) {
                        ctx.beginPath()
                        ctx.arc(12, 5 + i * 7, 2.1, 0, Math.PI * 2)
                        ctx.fill()
                    }
                } else if (btn.glyph === "refresh") {
                    ctx.beginPath()
                    ctx.arc(12, 12, 7.5, -Math.PI / 3, Math.PI * 1.35, false)
                    ctx.stroke()
                    // 화살촉
                    const a = -Math.PI / 3
                    const tx = 12 + 7.5 * Math.cos(a), ty = 12 + 7.5 * Math.sin(a)
                    ctx.beginPath()
                    ctx.moveTo(tx + 4.5, ty - 1.0)
                    ctx.lineTo(tx - 0.8, ty - 4.2)
                    ctx.lineTo(tx - 0.4, ty + 3.6)
                    ctx.closePath()
                    ctx.fill()
                }
            }

            Connections {
                target: btn
                function onIconColorChanged() { canvas.requestPaint() }
                function onGlyphChanged() { canvas.requestPaint() }
            }

            RotationAnimation on rotation {
                running: btn.spinning
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 800
                onRunningChanged: if (!running) canvas.rotation = 0
            }
        }
    }
}
