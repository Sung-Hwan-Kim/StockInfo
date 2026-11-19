#include "ChartItem.h"

#include "Skin.h"

#include <QGuiApplication>
#include <QLocale>
#include <QPainter>
#include <QtMath>

namespace {

int chartCount(const QString &tf)
{
    if (tf == QLatin1String("week"))
        return 300;
    if (tf == QLatin1String("month"))
        return 240;
    return 500;
}

ChartCanvas::Timeframe toTimeframe(const QString &tf)
{
    if (tf == QLatin1String("week"))
        return ChartCanvas::Timeframe::Week;
    if (tf == QLatin1String("month"))
        return ChartCanvas::Timeframe::Month;
    return ChartCanvas::Timeframe::Day;
}

qreal dist(const QPointF &a, const QPointF &b)
{
    return std::hypot(a.x() - b.x(), a.y() - b.y());
}

} // namespace

ChartItem::ChartItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptTouchEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setAntialiasing(true);
    setFillColor(Qt::transparent);
    m_canvas.setMessage(QStringLiteral("차트 불러오는 중…"));
}

// ── 속성 ──

void ChartItem::setService(QObject *s)
{
    auto *svc = qobject_cast<StockService *>(s);
    if (svc == m_service)
        return;
    if (m_service)
        disconnect(m_service, nullptr, this, nullptr);
    m_service = svc;
    if (m_service) {
        connect(m_service, &StockService::chartLoaded, this, &ChartItem::onChartLoaded);
        connect(m_service, &StockService::chartFailed, this, &ChartItem::onChartFailed);
        connect(m_service, &StockService::stocksUpdated, this, &ChartItem::onQuotes);
        connect(m_service, &StockService::indexesUpdated, this, &ChartItem::onQuotes);
    }
    emit serviceChanged();
    reload();
}

void ChartItem::setSymbol(const QString &s)
{
    if (s == m_symbol)
        return;
    m_symbol = s;
    m_daily.clear();
    m_stats.clear();
    emit statsChanged();
    emit symbolChanged();
    reload();
}

void ChartItem::setIsIndex(bool on)
{
    if (on == m_isIndex)
        return;
    m_isIndex = on;
    m_canvas.setVolumeLabel(on ? QStringLiteral("거래량(천주)") : QStringLiteral("거래량"));
    emit isIndexChanged();
    update();
}

void ChartItem::setTimeframe(const QString &tf)
{
    if (tf == m_timeframe)
        return;
    m_timeframe = tf;
    emit timeframeChanged();
    reload();
}

void ChartItem::setDarkMode(bool on)
{
    if (on == m_dark)
        return;
    m_dark = on;
    emit darkModeChanged();
    update();
}

void ChartItem::setShowMA(bool on) { m_canvas.setShowMA(on); emit indicatorsChanged(); update(); }
void ChartItem::setShowBollinger(bool on) { m_canvas.setShowBollinger(on); emit indicatorsChanged(); update(); }
void ChartItem::setShowVolume(bool on) { m_canvas.setShowVolume(on); emit indicatorsChanged(); update(); }
void ChartItem::setShowMACD(bool on) { m_canvas.setShowMACD(on); emit indicatorsChanged(); update(); }
void ChartItem::setShowRSI(bool on) { m_canvas.setShowRSI(on); emit indicatorsChanged(); update(); }
void ChartItem::setShowStochastic(bool on) { m_canvas.setShowStochastic(on); emit indicatorsChanged(); update(); }

void ChartItem::setLoading(bool on)
{
    if (on != m_loading) {
        m_loading = on;
        emit loadingChanged();
    }
}

// ── 데이터 ──

void ChartItem::reload()
{
    if (!m_service || m_symbol.isEmpty())
        return;
    setLoading(true);
    m_canvas.setMessage(QStringLiteral("차트 불러오는 중…"));
    update();
    m_service->fetchChart(m_symbol, m_timeframe, chartCount(m_timeframe));
    // 52주 최고/최저용 일봉 (주봉/월봉을 보고 있을 때)
    if (m_timeframe != QLatin1String("day") && m_daily.isEmpty())
        m_service->fetchChart(m_symbol, QStringLiteral("day"), 260);
}

void ChartItem::onChartLoaded(const QString &symbol, const QString &timeframe, const QVector<Candle> &candles,
                              int decimals)
{
    if (symbol != m_symbol)
        return;
    if (timeframe == QLatin1String("day")) {
        m_daily = candles;
        computeStats(candles);
    }
    if (timeframe != m_timeframe)
        return;

    // 폰처럼 좁으면 처음에 보이는 봉 수를 줄인다 (봉 하나에 약 6px)
    const int preferred = qBound(30, int(width() / 6.0), 120);
    m_canvas.setPreferredVisible(preferred);
    m_canvas.setBounds(boundingRect());
    m_canvas.setCandles(candles, toTimeframe(timeframe), decimals);
    setLoading(false);
    update();
}

void ChartItem::onChartFailed(const QString &symbol, const QString &timeframe, const QString &message)
{
    if (symbol != m_symbol || timeframe != m_timeframe)
        return;
    m_canvas.setMessage(message);
    setLoading(false);
    update();
}

void ChartItem::onQuotes(const QList<Quote> &quotes)
{
    for (const Quote &q : quotes) {
        if (q.code != m_symbol || !q.hasHighLow || q.priceValue <= 0)
            continue;
        const QDate today = q.tradedAt.isValid() ? q.tradedAt.date() : QDate::currentDate();
        // 지수 차트 거래량은 천 주 단위, 실시간 시세는 주 단위
        const double volume = m_isIndex ? q.volumeValue / 1000.0 : q.volumeValue;
        if (m_canvas.applyLive(today, q.openValue, q.highValue, q.lowValue, q.priceValue, volume))
            update();
    }
}

void ChartItem::computeStats(const QVector<Candle> &daily)
{
    double hi = 0, lo = 0;
    QDate hiDate, loDate;
    const QDate from = QDate::currentDate().addDays(-365);
    for (const Candle &c : daily) {
        if (c.date < from)
            continue;
        if (hi == 0 || c.high > hi) { hi = c.high; hiDate = c.date; }
        if (lo == 0 || c.low < lo) { lo = c.low; loDate = c.date; }
    }
    static const QLocale kr(QLocale::Korean, QLocale::SouthKorea);
    const int decimals = m_isIndex ? 2 : 0;
    m_stats.clear();
    if (hi > 0) {
        m_stats[QStringLiteral("high")] = kr.toString(hi, 'f', decimals);
        m_stats[QStringLiteral("highDate")] = hiDate.toString(QStringLiteral("yy.MM.dd"));
        m_stats[QStringLiteral("low")] = kr.toString(lo, 'f', decimals);
        m_stats[QStringLiteral("lowDate")] = loDate.toString(QStringLiteral("yy.MM.dd"));
    }
    emit statsChanged();
}

void ChartItem::resetView()
{
    m_canvas.resetView();
    m_canvas.clearHover();
    update();
}

// ── 그리기 ──

void ChartItem::paint(QPainter *painter)
{
    m_canvas.setBounds(boundingRect());
    // 안드로이드 기본 글꼴은 커서 차트가 답답하므로 12px로 고정 (PC 9pt와 비슷)
    QFont f = QGuiApplication::font();
    f.setPixelSize(12);
    m_canvas.setStyle(Skin::palette(m_dark ? Skin::Mode::Dark : Skin::Mode::Light), f);
    m_canvas.paint(*painter);
}

void ChartItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    m_canvas.setBounds(boundingRect());
    update();
}

// ── 터치 ──

void ChartItem::handleTap(const QPointF &pos)
{
    // 두 번 탭: 처음 보기
    if (m_lastTap.isValid() && m_lastTap.elapsed() < 350 && dist(pos, m_lastTapPos) < 40) {
        m_lastTap.invalidate();
        resetView();
        return;
    }
    m_lastTap.start();
    m_lastTapPos = pos;

    // 탭: 그 봉의 값 보기 (같은 봉을 다시 탭하면 숨김)
    const int idx = m_canvas.indexAt(pos.x());
    if (idx >= 0 && idx == m_canvas.hoverIndex())
        m_canvas.clearHover();
    else
        m_canvas.setHover(idx, pos);
    update();
}

void ChartItem::touchEvent(QTouchEvent *e)
{
    const auto &points = e->points();
    m_canvas.setBounds(boundingRect());

    switch (e->type()) {
    case QEvent::TouchBegin:
        if (points.isEmpty())
            break;
        m_gesture = Gesture::Pan;
        m_moved = false;
        m_touchStart = points.first().position();
        m_canvas.beginPan(m_touchStart.x());
        setKeepTouchGrab(true);
        e->accept();
        return;

    case QEvent::TouchUpdate:
        if (points.size() >= 2) {
            const QPointF a = points[0].position(), b = points[1].position();
            if (m_gesture != Gesture::Pinch) {
                m_gesture = Gesture::Pinch;
                m_moved = true;
                m_canvas.endPan();
                m_canvas.beginPinch();
                m_pinchStartDist = qMax<qreal>(1.0, dist(a, b));
            } else {
                m_canvas.pinchTo((a.x() + b.x()) / 2.0, dist(a, b) / m_pinchStartDist);
                update();
            }
        } else if (points.size() == 1 && m_gesture == Gesture::Pan) {
            const QPointF p = points.first().position();
            if (!m_moved && dist(p, m_touchStart) > 12) {
                m_moved = true;
                m_canvas.clearHover();
            }
            if (m_moved) {
                m_canvas.panTo(p.x());
                update();
            }
        }
        e->accept();
        return;

    case QEvent::TouchEnd:
        if (m_gesture == Gesture::Pan && !m_moved && !points.isEmpty())
            handleTap(points.first().position());
        m_canvas.endPan();
        m_gesture = Gesture::None;
        setKeepTouchGrab(false);
        e->accept();
        return;

    case QEvent::TouchCancel:
        m_canvas.endPan();
        m_gesture = Gesture::None;
        setKeepTouchGrab(false);
        e->accept();
        return;

    default:
        break;
    }
    QQuickPaintedItem::touchEvent(e);
}

// ── 마우스 (PC 미리보기) ──

void ChartItem::mousePressEvent(QMouseEvent *e)
{
    m_canvas.setBounds(boundingRect());
    m_canvas.beginPan(e->position().x());
    m_touchStart = e->position();
    m_moved = false;
    setKeepMouseGrab(true);
    e->accept();
}

void ChartItem::mouseMoveEvent(QMouseEvent *e)
{
    const QPointF p = e->position();
    if (!m_moved && dist(p, m_touchStart) > 6)
        m_moved = true;
    m_canvas.panTo(p.x());
    m_canvas.setHover(m_canvas.indexAt(p.x()), p);
    update();
}

void ChartItem::mouseReleaseEvent(QMouseEvent *)
{
    m_canvas.endPan();
    setKeepMouseGrab(false);
}

void ChartItem::mouseDoubleClickEvent(QMouseEvent *)
{
    resetView();
}

void ChartItem::wheelEvent(QWheelEvent *e)
{
    m_canvas.setBounds(boundingRect());
    const QPointF p = e->position();
    m_canvas.zoomSteps(p.x(), e->angleDelta().y() / 120.0);
    m_canvas.setHover(m_canvas.indexAt(p.x()), p);
    update();
}

void ChartItem::hoverMoveEvent(QHoverEvent *e)
{
    const QPointF p = e->position();
    m_canvas.setBounds(boundingRect());
    m_canvas.setHover(m_canvas.indexAt(p.x()), p);
    update();
}

void ChartItem::hoverLeaveEvent(QHoverEvent *)
{
    m_canvas.clearHover();
    update();
}
