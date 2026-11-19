#pragma once

#include "ChartCanvas.h"
#include "StockService.h"

#include <QElapsedTimer>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// QML용 차트. 그리기는 PC와 같은 ChartCanvas를 쓴다.
// 터치: 끌기 = 이동, 두 손가락 = 확대/축소, 탭 = 값 보기(다시 탭하면 숨김), 두 번 탭 = 처음 보기
// 마우스(PC 미리보기): 끌기 = 이동, 휠 = 확대/축소, 더블클릭 = 처음 보기
class ChartItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject *service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(QString symbol READ symbol WRITE setSymbol NOTIFY symbolChanged)
    Q_PROPERTY(bool isIndex READ isIndex WRITE setIsIndex NOTIFY isIndexChanged)
    Q_PROPERTY(QString timeframe READ timeframe WRITE setTimeframe NOTIFY timeframeChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(bool showMA READ showMA WRITE setShowMA NOTIFY indicatorsChanged)
    Q_PROPERTY(bool showBollinger READ showBollinger WRITE setShowBollinger NOTIFY indicatorsChanged)
    Q_PROPERTY(bool showVolume READ showVolume WRITE setShowVolume NOTIFY indicatorsChanged)
    Q_PROPERTY(bool showMACD READ showMACD WRITE setShowMACD NOTIFY indicatorsChanged)
    Q_PROPERTY(bool showRSI READ showRSI WRITE setShowRSI NOTIFY indicatorsChanged)
    Q_PROPERTY(bool showStochastic READ showStochastic WRITE setShowStochastic NOTIFY indicatorsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY statsChanged) // 52주 최고/최저

public:
    explicit ChartItem(QQuickItem *parent = nullptr);

    QObject *service() const { return m_service; }
    void setService(QObject *s);
    QString symbol() const { return m_symbol; }
    void setSymbol(const QString &s);
    bool isIndex() const { return m_isIndex; }
    void setIsIndex(bool on);
    QString timeframe() const { return m_timeframe; }
    void setTimeframe(const QString &tf);
    bool darkMode() const { return m_dark; }
    void setDarkMode(bool on);

    bool showMA() const { return m_canvas.showMA(); }
    bool showBollinger() const { return m_canvas.showBollinger(); }
    bool showVolume() const { return m_canvas.showVolume(); }
    bool showMACD() const { return m_canvas.showMACD(); }
    bool showRSI() const { return m_canvas.showRSI(); }
    bool showStochastic() const { return m_canvas.showStochastic(); }
    void setShowMA(bool on);
    void setShowBollinger(bool on);
    void setShowVolume(bool on);
    void setShowMACD(bool on);
    void setShowRSI(bool on);
    void setShowStochastic(bool on);

    bool loading() const { return m_loading; }
    QVariantMap stats() const { return m_stats; }

    Q_INVOKABLE void resetView();
    void paint(QPainter *painter) override;

signals:
    void serviceChanged();
    void symbolChanged();
    void isIndexChanged();
    void timeframeChanged();
    void darkModeChanged();
    void indicatorsChanged();
    void loadingChanged();
    void statsChanged();

protected:
    void touchEvent(QTouchEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void hoverMoveEvent(QHoverEvent *e) override;
    void hoverLeaveEvent(QHoverEvent *e) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void reload();
    void setLoading(bool on);
    void onChartLoaded(const QString &symbol, const QString &timeframe, const QVector<Candle> &candles, int decimals);
    void onChartFailed(const QString &symbol, const QString &timeframe, const QString &message);
    void onQuotes(const QList<Quote> &quotes);
    void computeStats(const QVector<Candle> &daily);
    void handleTap(const QPointF &pos);

    ChartCanvas m_canvas;
    QPointer<StockService> m_service;
    QString m_symbol;
    bool m_isIndex = false;
    QString m_timeframe = QStringLiteral("day");
    bool m_dark = true;
    bool m_loading = false;
    QVariantMap m_stats;
    QVector<Candle> m_daily;

    // 터치 상태
    enum class Gesture { None, Pan, Pinch };
    Gesture m_gesture = Gesture::None;
    QPointF m_touchStart;
    bool m_moved = false;
    qreal m_pinchStartDist = 0.0;
    QElapsedTimer m_lastTap;
    QPointF m_lastTapPos;
};
