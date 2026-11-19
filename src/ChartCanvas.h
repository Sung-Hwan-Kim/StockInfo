#pragma once

#include "ChartData.h"

#include <QFont>
#include <QPalette>
#include <QPointF>
#include <QRectF>
#include <QString>

class QPainter;

// 주식 차트 그리기 + 보기 범위(확대/이동) 계산. 위젯/QML 어디서든 쓸 수 있도록 QWidget에 의존하지 않는다.
//   PC:     ChartWidget (QWidget) 이 감싸서 사용
//   모바일: ChartItem (QQuickPaintedItem) 이 감싸서 사용
// 캔들 + 이동평균/볼린저, 거래량, MACD(OSC), RSI, 스토캐스틱
class ChartCanvas
{
public:
    enum class Timeframe { Day, Week, Month };

    // ── 데이터 ──
    void setCandles(const QVector<Candle> &candles, Timeframe tf, int decimals);
    void setMessage(const QString &message); // "불러오는 중…" 등 (빈 문자열이면 숨김)
    void setVolumeLabel(const QString &label) { m_volumeLabel = label; }
    // 실시간 시세로 마지막 봉 갱신 (오늘 날짜, 시/고/저/종가, 누적 거래량). 바뀌었으면 true
    bool applyLive(const QDate &date, double open, double high, double low, double close, double volume);
    bool isEmpty() const { return m_c.isEmpty(); }
    const QVector<Candle> &candles() const { return m_c; }
    Timeframe timeframe() const { return m_tf; }
    int decimals() const { return m_decimals; }

    // ── 표시할 지표 ──
    void setShowMA(bool on) { m_showMA = on; }
    void setShowBollinger(bool on) { m_showBB = on; }
    void setShowVolume(bool on) { m_showVol = on; }
    void setShowMACD(bool on) { m_showMACD = on; }
    void setShowRSI(bool on) { m_showRSI = on; }
    void setShowStochastic(bool on) { m_showStoch = on; }
    bool showMA() const { return m_showMA; }
    bool showBollinger() const { return m_showBB; }
    bool showVolume() const { return m_showVol; }
    bool showMACD() const { return m_showMACD; }
    bool showRSI() const { return m_showRSI; }
    bool showStochastic() const { return m_showStoch; }

    // ── 그리기 ──
    void setBounds(const QRectF &r) { m_bounds = r; }
    void setStyle(const QPalette &pal, const QFont &font) { m_pal = pal; m_font = font; }
    void paint(QPainter &p) const;

    // ── 상호작용 (좌표는 setBounds 기준) ──
    int indexAt(qreal x) const;
    void setHover(int index, const QPointF &pos) { m_hover = index; m_mouse = pos; }
    void clearHover() { m_hover = -1; }
    int hoverIndex() const { return m_hover; }

    void beginPan(qreal x);
    void panTo(qreal x);
    void endPan() { m_dragging = false; }
    bool isPanning() const { return m_dragging; }

    // factor < 1 이면 확대(보이는 봉 수 감소). x 위치를 기준으로 확대/축소
    void zoomAt(qreal x, double factor, bool forceChange = true);
    void zoomSteps(qreal x, double wheelSteps); // 마우스 휠 (위로 = 확대)
    // 두 손가락 확대/축소: 시작 시점 기준 scale(손가락 간격 비율)
    void beginPinch();
    void pinchTo(qreal centerX, double scale);

    void resetView();
    // 처음 보기(resetView)에서 보여줄 봉 수. 0이면 기본값(일/월봉 120, 주봉 104)
    void setPreferredVisible(int n) { m_preferredVisible = n; }

private:
    enum class PaneKind { Price, Volume, Macd, Rsi, Stoch };
    struct Pane
    {
        PaneKind kind;
        QRectF rect;
        double lo = 0.0;
        double hi = 1.0;
    };

    void recompute();
    void clampView();
    QRectF plotArea() const;
    QVector<Pane> layoutPanes() const;
    void computeRange(Pane &pane, int first, int last) const;
    double slotWidth() const;
    int firstVisible() const { return qMax(0, m_end - m_visible); }
    qreal xOf(int index) const;
    static qreal yOf(const Pane &pane, double v);

    void drawPane(QPainter &p, const Pane &pane, int first, int last) const;
    void drawPriceContent(QPainter &p, const Pane &pane, int first, int last) const;
    void drawVolumeContent(QPainter &p, const Pane &pane, int first, int last) const;
    void drawMacdContent(QPainter &p, const Pane &pane, int first, int last) const;
    void drawLineSeries(QPainter &p, const Pane &pane, const QVector<double> &v, int first, int last,
                        const QColor &color, qreal width = 1.3) const;
    void drawLegend(QPainter &p, const Pane &pane, int index) const;
    void drawDateAxis(QPainter &p, const QRectF &area, int first, int last) const;
    void drawCrosshair(QPainter &p, const QVector<Pane> &panes) const;

    QString fmtPrice(double v) const;
    static QString fmtVolume(double v);
    QString fmtDate(const QDate &d, bool longForm) const;
    int candleDirection(int i) const; // 1 상승 / -1 하락 / 0 보합

    QVector<Candle> m_c;
    Timeframe m_tf = Timeframe::Day;
    int m_decimals = 0;
    QString m_message;
    QString m_volumeLabel = QStringLiteral("거래량");

    // 지표 값
    QVector<double> m_ma[4];
    QVector<double> m_bbMid, m_bbUp, m_bbDn;
    QVector<double> m_macd, m_signal, m_osc;
    QVector<double> m_rsi;
    QVector<double> m_stK, m_stD;

    bool m_showMA = true;
    bool m_showBB = false;
    bool m_showVol = true;
    bool m_showMACD = true;
    bool m_showRSI = true;
    bool m_showStoch = false;

    // 보기 범위: [m_end - m_visible, m_end)
    int m_visible = 120;
    int m_end = 0;

    // 그리기 환경
    QRectF m_bounds;
    QPalette m_pal;
    QFont m_font;

    // 마우스/터치
    int m_hover = -1;
    QPointF m_mouse;
    bool m_dragging = false;
    qreal m_dragX = 0.0;
    int m_dragEnd = 0;
    int m_pinchVisible = 0;
    int m_pinchEnd = 0;
    int m_preferredVisible = 0;
};
