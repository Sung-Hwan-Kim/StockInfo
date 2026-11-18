#pragma once

#include "ChartData.h"

#include <QWidget>

// 주식 차트 (직접 그림): 캔들 + 이동평균/볼린저, 거래량, MACD, RSI, 스토캐스틱
// - 마우스 휠: 확대/축소, 끌기: 좌우 이동, 더블클릭: 처음 보기로
// - 마우스를 올리면 그 봉의 값(시가/고가/저가/종가, 지표 값)을 표시
class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    enum class Timeframe { Day, Week, Month };

    explicit ChartWidget(QWidget *parent = nullptr);

    void setCandles(const QVector<Candle> &candles, Timeframe tf, int decimals);
    void setMessage(const QString &message); // "불러오는 중…" 등 (빈 문자열이면 숨김)

    // 실시간 시세로 마지막 봉 갱신 (오늘 날짜, 시/고/저/종가, 누적 거래량)
    void applyLive(const QDate &date, double open, double high, double low, double close, double volume);

    // 표시할 지표
    void setShowMA(bool on);
    void setShowBollinger(bool on);
    void setShowVolume(bool on);
    void setShowMACD(bool on);
    void setShowRSI(bool on);
    void setShowStochastic(bool on);

    QSize sizeHint() const override { return {900, 560}; }
    QSize minimumSizeHint() const override { return {400, 260}; }

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void leaveEvent(QEvent *e) override;

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
    void resetView();
    void clampView();
    QRectF plotArea() const;
    QVector<Pane> layoutPanes() const;
    void computeRange(Pane &pane, int first, int last) const;
    double slotWidth() const;
    int firstVisible() const { return qMax(0, m_end - m_visible); }
    int indexAt(qreal x) const;
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

    // 마우스
    int m_hover = -1;
    QPointF m_mouse;
    bool m_dragging = false;
    qreal m_dragX = 0.0;
    int m_dragEnd = 0;
};
