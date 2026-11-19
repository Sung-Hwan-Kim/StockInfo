import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 지수 카드 (탭하면 지수 상세)
Rectangle {
    id: card
    property string title
    property var info: ({})
    signal clicked()

    readonly property int dir: info.direction || 0
    readonly property color dirColor: dir > 0 ? App.colors.up : dir < 0 ? App.colors.down : App.colors.text

    radius: 10
    color: area.pressed ? App.colors.pressed : App.colors.base
    border.color: App.colors.text
    border.width: 2

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 0

        Label {
            text: card.title
            color: App.colors.text
            font.bold: true
            font.pixelSize: Math.max(13, card.height * 0.13)
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: card.info.price || "-"
            color: card.dirColor
            font.bold: true
            font.pixelSize: Math.max(16, card.height * 0.27)
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            verticalAlignment: Text.AlignVCenter
        }
        Label {
            text: card.info.hasQuote ? card.info.changeText + "   " + card.info.rateText : ""
            color: card.dirColor
            font.bold: true
            font.pixelSize: Math.max(12, card.height * 0.12)
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 10
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
        Label {
            text: card.info.hasHighLow ? "고 " + card.info.high + "  저 " + card.info.low : (card.info.status || "")
            color: App.colors.dim
            font.pixelSize: Math.max(10, card.height * 0.085)
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 8
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
        Label {
            visible: card.info.hasHighLow === true
            text: card.info.status || ""
            color: App.colors.dim
            font.pixelSize: Math.max(10, card.height * 0.08)
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 8
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        onClicked: card.clicked()
    }
}
