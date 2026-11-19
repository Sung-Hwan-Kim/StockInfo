import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 종목 한 줄 (두 줄 표시)
//   종목명      현재가          고 199,950 +5.07%
//   종목코드    ▲ 1,200 +0.62%  저 188,800 -0.79%
ItemDelegate {
    id: row
    required property int index
    required property string code
    required property string name
    required property string price
    required property string changeText
    required property string rateText
    required property int direction
    required property bool hasHighLow
    required property string high
    required property string highRateText
    required property int highDir
    required property string low
    required property string lowRateText
    required property int lowDir

    property real priceWidth: 96
    property real hlWidth: 128

    function colorOf(d) { return d > 0 ? App.colors.up : d < 0 ? App.colors.down : App.colors.text }

    height: 58
    padding: 0
    leftPadding: 12
    rightPadding: 12

    background: Rectangle {
        color: row.pressed ? App.colors.pressed : (row.index % 2 ? App.colors.alt : App.colors.base)
    }

    contentItem: RowLayout {
        spacing: 8

        ColumnLayout {
            spacing: 1
            Layout.fillWidth: true
            Label {
                text: row.name
                color: App.colors.text
                font.pixelSize: 15
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Label {
                text: row.code
                color: App.colors.dim
                font.pixelSize: 11
            }
        }

        ColumnLayout {
            spacing: 1
            Layout.preferredWidth: row.priceWidth
            Layout.maximumWidth: row.priceWidth
            Label {
                text: row.price
                color: row.colorOf(row.direction)
                font.pixelSize: 15
                font.bold: true
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }
            Label {
                text: row.changeText + " " + row.rateText
                color: row.colorOf(row.direction)
                font.pixelSize: 11
                horizontalAlignment: Text.AlignRight
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: 8
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            spacing: 1
            Layout.preferredWidth: row.hlWidth
            Layout.maximumWidth: row.hlWidth
            Label {
                text: row.hasHighLow ? "고 " + row.high + "  " + row.highRateText : "고 -"
                color: row.hasHighLow ? row.colorOf(row.highDir) : App.colors.dim
                font.pixelSize: 12
                horizontalAlignment: Text.AlignRight
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: 8
                Layout.fillWidth: true
            }
            Label {
                text: row.hasHighLow ? "저 " + row.low + "  " + row.lowRateText : "저 -"
                color: row.hasHighLow ? row.colorOf(row.lowDir) : App.colors.dim
                font.pixelSize: 12
                horizontalAlignment: Text.AlignRight
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: 8
                Layout.fillWidth: true
            }
        }
    }
}
