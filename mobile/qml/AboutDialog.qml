import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 버전 정보
Dialog {
    id: dlg
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent ? parent.width - 32 : 360, 420)
    modal: true
    title: "버전 정보"
    standardButtons: Dialog.Close

    ColumnLayout {
        width: parent.width
        spacing: 8

        RowLayout {
            spacing: 14
            Image {
                source: "qrc:/icons/icon_128.png"
                sourceSize.width: 64
                sourceSize.height: 64
            }
            ColumnLayout {
                spacing: 2
                Label { text: "StockInfo"; font.pixelSize: 22; font.bold: true; color: App.colors.text }
                Label { text: "코스피 · 코스닥 주식 정보"; color: App.colors.text }
                Label { text: "버전 " + App.version; font.bold: true; color: App.colors.link }
            }
        }
        Label {
            text: App.buildInfo
            color: App.colors.dim
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            text: "시세 출처: 네이버 금융 (비공식 공개 API)"
            color: App.colors.text
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            text: "<a href=\"" + App.homepage + "\">" + App.homepage + "</a>"
            textFormat: Text.RichText
            linkColor: App.colors.link
            font.pixelSize: 13
            wrapMode: Text.WrapAnywhere
            Layout.fillWidth: true
            onLinkActivated: (link) => Qt.openUrlExternally(link)
        }
        Label {
            text: "표시되는 시세는 참고용이며 실제 시세와 차이가 있을 수 있습니다."
            color: App.colors.dim
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
