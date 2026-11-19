import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    // PC 미리보기 기본 크기 (폰). 안드로이드에서는 전체 화면
    width: 412
    height: 860
    visible: true
    title: "StockInfo"
    color: App.colors.window

    Material.theme: App.darkMode ? Material.Dark : Material.Light
    Material.accent: App.colors.accent
    Material.background: App.colors.window
    Material.foreground: App.colors.text

    // 화면 크기/방향 (폰·태블릿, 가로·세로 모두 지원)
    readonly property bool wide: width >= 720          // 태블릿 또는 가로 화면
    readonly property bool compactHeight: height < 560 // 폰 가로

    function openDetail(code, isIndex) {
        stack.push(detailComponent, { code: code, isIndex: isIndex })
    }

    header: ToolBar {
        Material.background: App.colors.base
        Material.foreground: App.colors.text
        height: 52

        RowLayout {
            anchors.fill: parent
            spacing: 0

            IconButton {
                visible: stack.depth > 1
                glyph: "back"
                onClicked: stack.pop()
            }
            Label {
                text: stack.depth > 1 && stack.currentItem ? stack.currentItem.title : "StockInfo"
                font.pixelSize: 19
                font.bold: true
                elide: Label.ElideRight
                leftPadding: stack.depth > 1 ? 0 : 16
                Layout.fillWidth: true
                color: App.colors.text
            }
            IconButton {
                glyph: "refresh"
                spinning: App.busy
                onClicked: App.refresh()
            }
            IconButton {
                glyph: "menu"
                onClicked: mainMenu.open()

                Menu {
                    id: mainMenu
                    y: parent.height
                    MenuItem {
                        text: "종목 추가 / 관리"
                        onTriggered: stack.push(settingsComponent)
                    }
                    MenuItem {
                        text: App.darkMode ? "라이트 모드로" : "다크 모드로"
                        onTriggered: App.darkMode = !App.darkMode
                    }
                    MenuItem {
                        text: "버전 정보"
                        onTriggered: aboutDialog.open()
                    }
                }
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: HomePage {
            wide: win.wide
            compactHeight: win.compactHeight
            onOpenDetail: (code, isIndex) => win.openDetail(code, isIndex)
        }
    }

    footer: Rectangle {
        height: 26
        color: App.colors.base
        Label {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            verticalAlignment: Text.AlignVCenter
            text: App.statusText
            color: App.colors.dim
            font.pixelSize: 12
            elide: Text.ElideRight
        }
    }

    Component {
        id: detailComponent
        DetailPage {
            wide: win.wide
            compactHeight: win.compactHeight
        }
    }
    Component {
        id: settingsComponent
        SettingsPage { }
    }

    AboutDialog {
        id: aboutDialog
    }

    // 안드로이드 뒤로 가기: 상세/설정 화면이면 이전 화면으로
    onClosing: (close) => {
        if (stack.depth > 1) {
            close.accepted = false
            stack.pop()
        }
    }
}
