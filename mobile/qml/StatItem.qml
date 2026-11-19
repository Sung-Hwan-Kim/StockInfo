import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 상세 화면의 "이름  값" 한 칸
RowLayout {
    id: item
    property string label
    property string value
    property color valueColor: App.colors.text
    spacing: 6

    Label {
        text: item.label
        color: App.colors.dim
        font.pixelSize: 12
        Layout.preferredWidth: 52
    }
    Label {
        text: item.value
        color: item.valueColor
        font.pixelSize: 13
        font.bold: true
        elide: Text.ElideRight
        Layout.fillWidth: true
    }
}
