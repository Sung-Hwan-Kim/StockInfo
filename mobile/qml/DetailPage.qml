import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import StockInfo

// 종목 / 지수 상세: 시세 요약 + 차트
//   세로 / 태블릿: 위에 시세 요약, 아래에 차트
//   폰 가로      : 왼쪽에 시세 요약(스크롤), 오른쪽에 큰 차트
Page {
    id: page
    property string code
    property bool isIndex: false
    property bool wide: false
    property bool compactHeight: false
    readonly property bool side: compactHeight && width > height

    // App.quotesRevision이 바뀔 때마다 다시 읽는다 (실시간 갱신)
    readonly property var q: { App.quotesRevision; return App.quote(page.code) }
    readonly property int dir: q.direction || 0

    function colorOf(d) { return d > 0 ? App.colors.up : d < 0 ? App.colors.down : App.colors.text }

    title: isIndex ? "지수 상세" : "종목 상세"
    background: Rectangle { color: App.colors.window }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 10
        columns: page.side ? 2 : 1
        rowSpacing: 6
        columnSpacing: 10

        // ── 시세 요약 + 버튼 ──
        Flickable {
            id: infoFlick
            Layout.fillWidth: !page.side
            Layout.fillHeight: page.side
            Layout.preferredWidth: page.side ? Math.min(400, page.width * 0.43) : -1
            Layout.preferredHeight: page.side ? -1 : info.implicitHeight
            contentHeight: info.implicitHeight
            interactive: contentHeight > height + 1
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            ScrollIndicator.vertical: ScrollIndicator { }

            ColumnLayout {
                id: info
                width: infoFlick.width
                spacing: 6

                // 이름 / 현재가
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    ColumnLayout {
                        spacing: 0
                        Layout.fillWidth: true
                        Label {
                            text: page.q.name || page.code
                            color: App.colors.text
                            font.pixelSize: 22
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: page.code + " · " + (page.q.marketLabel || "")
                            color: App.colors.dim
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    ColumnLayout {
                        spacing: 0
                        Label {
                            text: page.q.price || "-"
                            color: page.colorOf(page.dir)
                            font.pixelSize: 26
                            font.bold: true
                            Layout.alignment: Qt.AlignRight
                        }
                        Label {
                            text: page.q.hasQuote ? page.q.changeText + "   " + page.q.rateText : ""
                            color: page.colorOf(page.dir)
                            font.pixelSize: 13
                            font.bold: true
                            Layout.alignment: Qt.AlignRight
                        }
                    }
                }
                Label {
                    visible: text.length > 0
                    text: page.q.status || ""
                    color: App.colors.dim
                    font.pixelSize: 12
                }

                // 시세 요약
                GridLayout {
                    Layout.fillWidth: true
                    columns: (!page.side && page.width >= 640) ? 4 : 2
                    columnSpacing: 14
                    rowSpacing: 2

                    StatItem { label: "시가"; value: page.q.open || "-"; Layout.fillWidth: true }
                    StatItem {
                        label: "고가"
                        value: page.q.hasHighLow ? page.q.high + " (" + page.q.highRateText + ")" : "-"
                        valueColor: page.q.hasHighLow ? page.colorOf(page.q.highDir) : App.colors.text
                        Layout.fillWidth: true
                    }
                    StatItem {
                        label: "저가"
                        value: page.q.hasHighLow ? page.q.low + " (" + page.q.lowRateText + ")" : "-"
                        valueColor: page.q.hasHighLow ? page.colorOf(page.q.lowDir) : App.colors.text
                        Layout.fillWidth: true
                    }
                    StatItem { label: "전일 종가"; value: page.q.prevClose || "-"; Layout.fillWidth: true }
                    StatItem { label: "거래량"; value: page.q.volume || "-"; Layout.fillWidth: true }
                    StatItem { label: "거래대금"; value: page.q.tradeValue || "-"; Layout.fillWidth: true }
                    StatItem {
                        visible: !page.isIndex
                        label: "시가총액"
                        value: page.q.marketCap || "-"
                        Layout.fillWidth: true
                    }
                    StatItem {
                        label: "52주"
                        value: chart.stats.high ? chart.stats.high + " / " + chart.stats.low : "불러오는 중…"
                        Layout.fillWidth: true
                        Layout.columnSpan: page.isIndex ? 2 : 1
                    }
                }

                // 봉 주기 + 네이버 금융
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Repeater {
                        model: [["day", "일봉"], ["week", "주봉"], ["month", "월봉"]]
                        delegate: Button {
                            required property var modelData
                            text: modelData[1]
                            checkable: true
                            checked: App.chartTimeframe === modelData[0]
                            highlighted: checked
                            flat: !checked
                            leftPadding: 14
                            rightPadding: 14
                            Layout.minimumWidth: 60
                            onClicked: App.chartTimeframe = modelData[0]
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "네이버"
                        flat: true
                        onClicked: App.openNaver(page.code, page.isIndex)
                    }
                }

                // 보조지표 켜고 끄기
                Flow {
                    Layout.fillWidth: true
                    spacing: 2
                    Repeater {
                        model: [["MA", "이동평균"], ["Bollinger", "볼린저"], ["Volume", "거래량"],
                                ["MACD", "MACD"], ["RSI", "RSI"], ["Stochastic", "스토캐스틱"]]
                        delegate: Button {
                            required property var modelData
                            text: modelData[1]
                            checkable: true
                            checked: App.chartIndicators[modelData[0]] === true
                            flat: true
                            highlighted: checked
                            font.bold: checked
                            font.pixelSize: 13
                            leftPadding: 9
                            rightPadding: 9
                            implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
                            onToggled: App.setChartIndicator(modelData[0], checked)
                        }
                    }
                }

                Label {
                    text: "끌기: 이동 · 두 손가락: 확대/축소 · 탭: 값 보기 · 두 번 탭: 처음 보기"
                    color: App.colors.dim
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // ── 차트 ──
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            color: App.colors.base
            radius: 6
            border.color: App.colors.border

            ChartItem {
                id: chart
                anchors.fill: parent
                anchors.margins: 2
                service: App.service
                symbol: page.code
                isIndex: page.isIndex
                timeframe: App.chartTimeframe
                darkMode: App.darkMode
                showMA: App.chartIndicators.MA === true
                showBollinger: App.chartIndicators.Bollinger === true
                showVolume: App.chartIndicators.Volume === true
                showMACD: App.chartIndicators.MACD === true
                showRSI: App.chartIndicators.RSI === true
                showStochastic: App.chartIndicators.Stochastic === true
            }
            BusyIndicator {
                anchors.centerIn: parent
                running: chart.loading
                visible: chart.loading
            }
        }
    }
}
