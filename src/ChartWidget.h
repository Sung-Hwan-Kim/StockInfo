#pragma once

#include "ChartCanvas.h"

#include <QWidget>

// PC용 차트 위젯. 그리기와 확대/이동 계산은 ChartCanvas가 하고, 이 위젯은 마우스 입력만 전달한다.
// - 마우스 휠: 확대/축소, 끌기: 좌우 이동, 더블클릭: 처음 보기로
// - 마우스를 올리면 그 봉의 값(시가/고가/저가/종가, 지표 값)을 표시
class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    using Timeframe = ChartCanvas::Timeframe;

    explicit ChartWidget(QWidget *parent = nullptr);

    void setCandles(const QVector<Candle> &candles, Timeframe tf, int decimals);
    void setMessage(const QString &message);
    void setVolumeLabel(const QString &label);
    void applyLive(const QDate &date, double open, double high, double low, double close, double volume);

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
    void resizeEvent(QResizeEvent *e) override;

private:
    ChartCanvas m_canvas;
};
