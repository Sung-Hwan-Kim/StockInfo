import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// 설정 + 종목 검색/추가/삭제/순서
Page {
    id: page
    title: "종목 추가 / 관리"
    background: Rectangle { color: App.colors.window }

    Flickable {
        anchors.fill: parent
        contentHeight: content.implicitHeight + 32
        clip: true
        ScrollIndicator.vertical: ScrollIndicator { }

        ColumnLayout {
            id: content
            x: 16
            y: 12
            width: parent.width - 32
            spacing: 10

            // ── 일반 ──
            Label { text: "일반"; color: App.colors.accent; font.bold: true; font.pixelSize: 14 }
            RowLayout {
                Layout.fillWidth: true
                Label { text: "갱신 시간 (초)"; color: App.colors.text; Layout.fillWidth: true }
                SpinBox {
                    from: 1
                    to: 3600
                    value: App.refreshSeconds
                    editable: true
                    onValueModified: App.refreshSeconds = value
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Label { text: "다크 모드"; color: App.colors.text; Layout.fillWidth: true }
                Switch {
                    checked: App.darkMode
                    onToggled: App.darkMode = checked
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: App.colors.border }

            // ── 종목 검색 ──
            Label { text: "종목 검색"; color: App.colors.accent; font.bold: true; font.pixelSize: 14 }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "종목명 또는 코드 (예: 삼성전자, 005930)"
                    inputMethodHints: Qt.ImhNoPredictiveText
                    onAccepted: App.search(text)
                }
                Button {
                    text: "검색"
                    highlighted: true
                    enabled: !App.searching
                    onClicked: App.search(searchField.text)
                }
            }
            Label {
                visible: text.length > 0
                text: App.searching ? "검색 중…" : App.searchMessage
                color: App.colors.dim
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Repeater {
                model: App.searchResults
                delegate: ItemDelegate {
                    required property var modelData
                    readonly property bool added: { App.stocks; return App.hasStock(modelData.code) }
                    Layout.fillWidth: true
                    height: 48
                    enabled: !added
                    contentItem: RowLayout {
                        Label {
                            text: modelData.name + "  (" + modelData.code + ", " + modelData.marketLabel + ")"
                            color: App.colors.text
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: added ? "등록됨" : "+ 추가"
                            color: added ? App.colors.dim : App.colors.accent
                            font.bold: !added
                        }
                    }
                    onClicked: App.addStock(modelData.code, modelData.name, modelData.market)
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: App.colors.border }

            // ── 등록된 종목 ──
            Label {
                text: "등록된 종목 (" + App.stocks.length + ")"
                color: App.colors.accent
                font.bold: true
                font.pixelSize: 14
            }
            Repeater {
                model: App.stocks
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true
                    height: 50
                    radius: 6
                    color: index % 2 ? App.colors.alt : App.colors.base
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 4
                        spacing: 0
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            Label { text: modelData.name; color: App.colors.text; font.pixelSize: 15; elide: Text.ElideRight; Layout.fillWidth: true }
                            Label { text: modelData.code + " · " + modelData.marketLabel; color: App.colors.dim; font.pixelSize: 11 }
                        }
                        ToolButton { text: "▲"; enabled: index > 0; onClicked: App.moveStock(modelData.code, -1) }
                        ToolButton { text: "▼"; enabled: index < App.stocks.length - 1; onClicked: App.moveStock(modelData.code, 1) }
                        ToolButton {
                            text: "삭제"
                            Material.foreground: App.colors.up
                            onClicked: App.removeStock(modelData.code)
                        }
                    }
                }
            }
        }
    }
}
