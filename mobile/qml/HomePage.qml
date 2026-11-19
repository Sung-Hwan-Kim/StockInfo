import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// 첫 화면: 코스피/코스닥 지수 카드 + 종목 목록. 화면 방향/크기에 따라 배치가 바뀐다.
//   폰 세로        : 지수 카드(위) + [코스피 | 코스닥] 탭 목록
//   폰 가로        : 지수 카드 2개를 왼쪽에 세로로 + 오른쪽 탭 목록
//   태블릿 세로/가로: 지수 카드(위) + 좌우 2단 목록 (PC와 같은 배치)
Page {
    id: page
    title: "StockInfo"
    property bool wide: false          // 폭이 넓음 (태블릿, 가로 화면)
    property bool compactHeight: false // 높이가 낮음 (폰 가로)
    signal openDetail(string code, bool isIndex)

    readonly property string mode: (compactHeight && width > height) ? "side" : (wide ? "wide" : "tabbed")

    background: Rectangle { color: App.colors.window }

    Loader {
        anchors.fill: parent
        anchors.margins: 8
        sourceComponent: page.mode === "side" ? sideLayout : page.mode === "wide" ? wideLayout : tabbedLayout
    }

    // ── 지수 카드 ──
    // (인라인 컴포넌트는 page id를 볼 수 없으므로 클릭 처리는 쓰는 쪽에서)
    component KospiCard: IndexCard {
        title: "코스피"
        info: App.kospi
    }
    component KosdaqCard: IndexCard {
        title: "코스닥"
        info: App.kosdaq
    }

    // ── [코스피 | 코스닥] 탭 목록 ──
    component TabbedLists: ColumnLayout {
        id: tabbed
        signal openDetail(string code)
        spacing: 6
        TabBar {
            id: tabs
            Layout.fillWidth: true
            Material.background: App.colors.window
            currentIndex: swipe.currentIndex
            TabButton { text: "코스피  " + App.kospiModel.count }
            TabButton { text: "코스닥  " + App.kosdaqModel.count }
        }
        SwipeView {
            id: swipe
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex
            clip: true
            StockList {
                title: "코스피"
                model: App.kospiModel
                onOpenDetail: (code) => tabbed.openDetail(code)
            }
            StockList {
                title: "코스닥"
                model: App.kosdaqModel
                onOpenDetail: (code) => tabbed.openDetail(code)
            }
        }
    }

    // 폰 세로
    Component {
        id: tabbedLayout
        ColumnLayout {
            spacing: 8
            RowLayout {
                readonly property real cardHeight: Math.min(150, page.height * 0.2)
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.preferredHeight: cardHeight
                Layout.maximumHeight: cardHeight
                spacing: 8
                KospiCard { Layout.fillWidth: true; Layout.fillHeight: true; onClicked: page.openDetail("KOSPI", true) }
                KosdaqCard { Layout.fillWidth: true; Layout.fillHeight: true; onClicked: page.openDetail("KOSDAQ", true) }
            }
            TabbedLists {
                Layout.fillWidth: true
                Layout.fillHeight: true
                onOpenDetail: (code) => page.openDetail(code, false)
            }
        }
    }

    // 폰 가로
    Component {
        id: sideLayout
        RowLayout {
            spacing: 8
            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: false
                Layout.preferredWidth: Math.min(300, page.width * 0.34)
                spacing: 8
                KospiCard { Layout.fillWidth: true; Layout.fillHeight: true; onClicked: page.openDetail("KOSPI", true) }
                KosdaqCard { Layout.fillWidth: true; Layout.fillHeight: true; onClicked: page.openDetail("KOSDAQ", true) }
            }
            TabbedLists {
                Layout.fillWidth: true
                Layout.fillHeight: true
                onOpenDetail: (code) => page.openDetail(code, false)
            }
        }
    }

    // 태블릿 세로/가로
    Component {
        id: wideLayout
        ColumnLayout {
            spacing: 8
            RowLayout {
                readonly property real cardHeight: Math.min(200, page.height * 0.24)
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.preferredHeight: cardHeight
                Layout.maximumHeight: cardHeight
                spacing: 8
                KospiCard { Layout.fillWidth: true; Layout.fillHeight: true; onClicked: page.openDetail("KOSPI", true) }
                KosdaqCard { Layout.fillWidth: true; Layout.fillHeight: true; onClicked: page.openDetail("KOSDAQ", true) }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8
                StockList {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "코스피"
                    showTitle: true
                    model: App.kospiModel
                    onOpenDetail: (code) => page.openDetail(code, false)
                }
                StockList {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "코스닥"
                    showTitle: true
                    model: App.kosdaqModel
                    onOpenDetail: (code) => page.openDetail(code, false)
                }
            }
        }
    }
}
