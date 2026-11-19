import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 종목 목록 (회색 테두리 리스트박스). 아래로 당기면 새로고침, 길게 누르면 메뉴
Rectangle {
    id: root
    property alias model: list.model
    property string title
    property bool showTitle: false
    signal openDetail(string code)

    // 열 너비 (폰 기준, 넓으면 늘어남)
    readonly property real priceWidth: Math.max(96, width * 0.26)
    readonly property real hlWidth: Math.max(128, width * 0.34)

    color: App.colors.base
    radius: 8
    border.color: "#8a8a8a"
    border.width: 2
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 2
        spacing: 0

        // 머리글
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: App.colors.header
            radius: 6
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8
                Label {
                    text: root.showTitle ? root.title + " 종목" : "종목명"
                    color: App.colors.dim
                    font.pixelSize: 12
                    font.bold: root.showTitle
                    Layout.fillWidth: true
                }
                Label {
                    text: "현재가 / 등락"
                    color: App.colors.dim
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignRight
                    Layout.preferredWidth: root.priceWidth
                }
                Label {
                    text: "고가 / 저가"
                    color: App.colors.dim
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignRight
                    Layout.preferredWidth: root.hlWidth
                }
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.DragOverBounds

            delegate: StockRow {
                width: ListView.view.width
                priceWidth: root.priceWidth
                hlWidth: root.hlWidth
                onClicked: root.openDetail(code)
                onPressAndHold: {
                    rowMenu.code = code
                    rowMenu.name = name
                    rowMenu.popup()
                }
            }

            ScrollIndicator.vertical: ScrollIndicator { }

            // 아래로 당겨서 새로고침
            onDragEnded: if (contentY < -70) App.refresh()
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                y: -list.contentY - height - 6
                visible: list.contentY < -10
                opacity: Math.min(1, -list.contentY / 70)
                text: list.contentY < -70 ? "놓으면 새로고침" : "당겨서 새로고침"
                color: App.colors.dim
                font.pixelSize: 12
            }
        }
    }

    Label {
        anchors.centerIn: parent
        visible: list.count === 0
        horizontalAlignment: Text.AlignHCenter
        text: "종목이 없습니다.\n오른쪽 위 메뉴 → 종목 추가 / 관리"
        color: App.colors.dim
        font.pixelSize: 14
    }

    Menu {
        id: rowMenu
        property string code
        property string name
        MenuItem {
            text: "상세 보기 (차트)"
            font.bold: true
            onTriggered: root.openDetail(rowMenu.code)
        }
        MenuItem {
            text: "네이버 금융에서 보기"
            onTriggered: App.openNaver(rowMenu.code, false)
        }
        MenuItem {
            text: "'" + rowMenu.name + "' 삭제"
            onTriggered: confirmDelete.open()
        }
    }

    Dialog {
        id: confirmDelete
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: "종목 삭제"
        standardButtons: Dialog.Yes | Dialog.No
        Label { text: "'" + rowMenu.name + "' 종목을 목록에서 삭제할까요?"; wrapMode: Text.WordWrap }
        onAccepted: App.removeStock(rowMenu.code)
    }
}
