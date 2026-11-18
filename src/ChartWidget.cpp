#include "ChartWidget.h"
#include "Theme.h"

#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QtMath>
#include <limits>

namespace {

constexpr qreal kAxisW = 80.0;  // 오른쪽 값 축 폭
constexpr qreal kDateH = 22.0;  // 아래 날짜 축 높이
constexpr qreal kGap = 8.0;     // 영역 사이 간격
constexpr qreal kLegendH = 17.0;

constexpr int kMaPeriods[4] = {5, 20, 60, 120};

QColor maColor(int i)
{
    static const QColor colors[4] = {QColor(0xe0, 0x56, 0x9b), QColor(0xf2, 0x9d, 0x1f),
                                     QColor(0x2f, 0xb3, 0x7a), QColor(0x8b, 0x7c, 0xf6)};
    return colors[i];
}
const QColor kBollinger(0x5b, 0x9b, 0xd5);
const QColor kMacdColor(0xf2, 0x9d, 0x1f);
const QColor kSignalColor(0x8b, 0x7c, 0xf6);
const QColor kRsiColor(0x2f, 0xb3, 0x7a);
const QColor kStochK(0xe0, 0x56, 0x9b);
const QColor kStochD(0x5b, 0x9b, 0xd5);

const QLocale &krLocale()
{
    static const QLocale l(QLocale::Korean, QLocale::SouthKorea);
    return l;
}

double niceStep(double range, int ticks)
{
    if (range <= 0)
        return 1.0;
    const double raw = range / ticks;
    const double mag = qPow(10.0, qFloor(std::log10(raw)));
    const double norm = raw / mag;
    const double step = norm < 1.5 ? 1.0 : norm < 3.0 ? 2.0 : norm < 7.0 ? 5.0 : 10.0;
    return step * mag;
}

QString fmtIndicator(double v)
{
    if (qIsNaN(v))
        return QStringLiteral("-");
    if (qAbs(v) >= 1000)
        return krLocale().toString(v, 'f', 0);
    return krLocale().toString(v, 'f', 2);
}

} // namespace

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

// ── 데이터 ──

void ChartWidget::setCandles(const QVector<Candle> &candles, Timeframe tf, int decimals)
{
    m_c = candles;
    m_tf = tf;
    m_decimals = decimals;
    m_message.clear();
    recompute();
    resetView();
    update();
}

void ChartWidget::setMessage(const QString &message)
{
    m_message = message;
    update();
}

void ChartWidget::applyLive(const QDate &date, double open, double high, double low, double close, double volume)
{
    if (m_c.isEmpty() || !date.isValid() || close <= 0)
        return;
    Candle &last = m_c.last();
    const bool atEnd = (m_end == m_c.size());

    auto sameWeek = [](const QDate &a, const QDate &b) {
        return a.addDays(1 - a.dayOfWeek()) == b.addDays(1 - b.dayOfWeek());
    };

    if (m_tf == Timeframe::Day) {
        if (date == last.date) {
            if (open > 0)
                last.open = open;
            if (high > 0)
                last.high = high;
            if (low > 0)
                last.low = low;
            last.close = close;
            if (volume > 0)
                last.volume = volume;
        } else if (date > last.date && high > 0 && low > 0) {
            Candle c;
            c.date = date;
            c.open = open > 0 ? open : close;
            c.high = high;
            c.low = low;
            c.close = close;
            c.volume = volume;
            m_c.append(c);
        } else {
            return;
        }
    } else {
        const bool samePeriod = (m_tf == Timeframe::Week)
            ? sameWeek(date, last.date)
            : (date.year() == last.date.year() && date.month() == last.date.month());
        if (!samePeriod || date < last.date)
            return;
        last.close = close;
        if (high > 0)
            last.high = qMax(last.high, high);
        if (low > 0)
            last.low = qMin(last.low, low);
    }

    recompute();
    if (atEnd)
        m_end = m_c.size();
    clampView();
    update();
}

void ChartWidget::recompute()
{
    const int n = int(m_c.size());
    QVector<double> close(n), high(n), low(n);
    for (int i = 0; i < n; ++i) {
        close[i] = m_c[i].close;
        high[i] = m_c[i].high;
        low[i] = m_c[i].low;
    }
    for (int k = 0; k < 4; ++k)
        m_ma[k] = Indicators::sma(close, kMaPeriods[k]);
    Indicators::bollinger(close, 20, 2.0, m_bbMid, m_bbUp, m_bbDn);
    Indicators::macd(close, 12, 26, 9, m_macd, m_signal, m_osc);
    m_rsi = Indicators::rsi(close, 14);
    Indicators::stochasticSlow(high, low, close, 12, 5, 5, m_stK, m_stD);
}

// ── 보기 범위 ──

void ChartWidget::resetView()
{
    const int def = (m_tf == Timeframe::Week) ? 104 : 120;
    m_visible = def;
    m_end = int(m_c.size());
    clampView();
}

void ChartWidget::clampView()
{
    const int n = int(m_c.size());
    if (n == 0) {
        m_visible = 0;
        m_end = 0;
        return;
    }
    const int minVis = qMin(20, n);
    m_visible = qBound(minVis, m_visible, n);
    m_end = qBound(m_visible, m_end, n);
}

void ChartWidget::setShowMA(bool on) { m_showMA = on; update(); }
void ChartWidget::setShowBollinger(bool on) { m_showBB = on; update(); }
void ChartWidget::setShowVolume(bool on) { m_showVol = on; update(); }
void ChartWidget::setShowMACD(bool on) { m_showMACD = on; update(); }
void ChartWidget::setShowRSI(bool on) { m_showRSI = on; update(); }
void ChartWidget::setShowStochastic(bool on) { m_showStoch = on; update(); }

// ── 좌표 ──

QRectF ChartWidget::plotArea() const
{
    return QRectF(rect()).adjusted(6, 4, -kAxisW, -kDateH);
}

double ChartWidget::slotWidth() const
{
    return m_visible > 0 ? plotArea().width() / m_visible : 1.0;
}

qreal ChartWidget::xOf(int index) const
{
    return plotArea().left() + (index - firstVisible() + 0.5) * slotWidth();
}

int ChartWidget::indexAt(qreal x) const
{
    const QRectF plot = plotArea();
    if (m_c.isEmpty() || x < plot.left() || x > plot.right())
        return -1;
    const int i = firstVisible() + int((x - plot.left()) / slotWidth());
    return qBound(firstVisible(), i, m_end - 1);
}

qreal ChartWidget::yOf(const Pane &pane, double v)
{
    if (pane.hi <= pane.lo)
        return pane.rect.center().y();
    return pane.rect.bottom() - (v - pane.lo) / (pane.hi - pane.lo) * pane.rect.height();
}

QVector<ChartWidget::Pane> ChartWidget::layoutPanes() const
{
    struct Item { PaneKind kind; double weight; };
    QVector<Item> items;
    items.append({PaneKind::Price, 5.0});
    if (m_showVol)
        items.append({PaneKind::Volume, 1.3});
    if (m_showMACD)
        items.append({PaneKind::Macd, 1.5});
    if (m_showRSI)
        items.append({PaneKind::Rsi, 1.2});
    if (m_showStoch)
        items.append({PaneKind::Stoch, 1.2});

    double total = 0.0;
    for (const Item &it : items)
        total += it.weight;

    const QRectF plot = plotArea();
    const double avail = plot.height() - kGap * (items.size() - 1);
    QVector<Pane> panes;
    double y = plot.top();
    for (const Item &it : items) {
        const double h = avail * it.weight / total;
        Pane pane;
        pane.kind = it.kind;
        // 위쪽에 범례 공간을 남긴다 (가격 영역은 두 줄)
        const double legend = (it.kind == PaneKind::Price) ? kLegendH * 2 : kLegendH;
        pane.rect = QRectF(plot.left(), y + legend, plot.width(), qMax(10.0, h - legend - 3));
        panes.append(pane);
        y += h + kGap;
    }
    return panes;
}

void ChartWidget::computeRange(Pane &pane, int first, int last) const
{
    double lo = std::numeric_limits<double>::max();
    double hi = std::numeric_limits<double>::lowest();
    auto take = [&](double v) {
        if (!qIsNaN(v)) {
            lo = qMin(lo, v);
            hi = qMax(hi, v);
        }
    };

    switch (pane.kind) {
    case PaneKind::Price:
        for (int i = first; i < last; ++i) {
            take(m_c[i].low);
            take(m_c[i].high);
            if (m_showMA)
                for (int k = 0; k < 4; ++k)
                    take(m_ma[k][i]);
            if (m_showBB) {
                take(m_bbUp[i]);
                take(m_bbDn[i]);
            }
        }
        break;
    case PaneKind::Volume:
        lo = 0.0;
        hi = 0.0;
        for (int i = first; i < last; ++i)
            hi = qMax(hi, m_c[i].volume);
        pane.lo = 0.0;
        pane.hi = hi > 0 ? hi * 1.08 : 1.0;
        return;
    case PaneKind::Macd:
        take(0.0);
        for (int i = first; i < last; ++i) {
            take(m_macd[i]);
            take(m_signal[i]);
            take(m_osc[i]);
        }
        break;
    case PaneKind::Rsi:
    case PaneKind::Stoch:
        pane.lo = 0.0;
        pane.hi = 100.0;
        return;
    }

    if (lo > hi) {
        lo = 0.0;
        hi = 1.0;
    }
    if (hi - lo < 1e-9) {
        const double d = qMax(qAbs(hi) * 0.01, 1.0);
        lo -= d;
        hi += d;
    }
    const double pad = (hi - lo) * (pane.kind == PaneKind::Price ? 0.07 : 0.1);
    pane.lo = lo - pad;
    pane.hi = hi + pad;
}

int ChartWidget::candleDirection(int i) const
{
    const Candle &c = m_c[i];
    if (c.close > c.open)
        return 1;
    if (c.close < c.open)
        return -1;
    if (i > 0) {
        if (c.close > m_c[i - 1].close)
            return 1;
        if (c.close < m_c[i - 1].close)
            return -1;
    }
    return 0;
}

// ── 서식 ──

QString ChartWidget::fmtPrice(double v) const
{
    if (qIsNaN(v))
        return QStringLiteral("-");
    return krLocale().toString(v, 'f', m_decimals);
}

QString ChartWidget::fmtVolume(double v)
{
    if (v >= 1e8)
        return krLocale().toString(v / 1e8, 'f', 1) + QStringLiteral("억");
    if (v >= 1e4)
        return krLocale().toString(v / 1e4, 'f', 0) + QStringLiteral("만");
    return krLocale().toString(v, 'f', 0);
}

QString ChartWidget::fmtDate(const QDate &d, bool longForm) const
{
    if (longForm) {
        if (m_tf == Timeframe::Month)
            return d.toString(QStringLiteral("yyyy-MM"));
        return d.toString(QStringLiteral("yyyy-MM-dd"));
    }
    switch (m_tf) {
    case Timeframe::Day:
        return d.toString(QStringLiteral("MM/dd"));
    case Timeframe::Week:
        return d.toString(QStringLiteral("yy/MM/dd"));
    case Timeframe::Month:
        return d.toString(QStringLiteral("yyyy/MM"));
    }
    return d.toString(Qt::ISODate);
}

// ── 그리기 ──

void ChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPalette pal = palette();
    p.fillRect(rect(), pal.color(QPalette::Base));

    if (m_c.isEmpty() || m_visible <= 0) {
        p.setPen(Theme::dimText(pal));
        QFont f = font();
        f.setPointSizeF(f.pointSizeF() * 1.2);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter, m_message.isEmpty() ? tr("차트 데이터가 없습니다.") : m_message);
        return;
    }

    const int first = firstVisible();
    const int last = m_end;
    QVector<Pane> panes = layoutPanes();
    for (Pane &pane : panes)
        computeRange(pane, first, last);

    for (const Pane &pane : std::as_const(panes))
        drawPane(p, pane, first, last);

    drawDateAxis(p, plotArea(), first, last);
    drawCrosshair(p, panes);

    if (!m_message.isEmpty()) {
        QFont f = font();
        f.setBold(true);
        p.setFont(f);
        const QFontMetrics fm(f);
        const QRectF box = QRectF(0, 0, fm.horizontalAdvance(m_message) + 28, fm.height() + 14)
                               .translated(rect().center() - QPointF((fm.horizontalAdvance(m_message) + 28) / 2.0,
                                                                     (fm.height() + 14) / 2.0));
        QColor bg = pal.color(QPalette::Window);
        bg.setAlpha(230);
        p.setPen(pal.color(QPalette::Mid));
        p.setBrush(bg);
        p.drawRoundedRect(box, 6, 6);
        p.setPen(pal.color(QPalette::WindowText));
        p.drawText(box, Qt::AlignCenter, m_message);
    }
}

void ChartWidget::drawPane(QPainter &p, const Pane &pane, int first, int last) const
{
    const QPalette pal = palette();
    const QColor gridColor = Theme::isDark(pal) ? QColor(255, 255, 255, 22) : QColor(0, 0, 0, 20);
    const QColor axisText = Theme::dimText(pal);

    // 테두리
    const double legend = (pane.kind == PaneKind::Price) ? kLegendH * 2 : kLegendH;
    const QRectF frame = pane.rect.adjusted(0, -legend, 0, 3);
    p.setPen(QPen(Theme::isDark(pal) ? QColor(255, 255, 255, 40) : QColor(0, 0, 0, 45), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(frame);

    // 가로 격자 + 오른쪽 값
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.86);
    p.setFont(small);
    const QFontMetrics fm(small);

    QVector<double> ticks;
    if (pane.kind == PaneKind::Rsi) {
        ticks = {30.0, 50.0, 70.0};
    } else if (pane.kind == PaneKind::Stoch) {
        ticks = {20.0, 50.0, 80.0};
    } else {
        const int target = pane.kind == PaneKind::Price ? qMax(3, int(pane.rect.height() / 55))
                         : pane.kind == PaneKind::Macd  ? 3
                                                        : 2;
        const double step = niceStep(pane.hi - pane.lo, target);
        for (double t = qCeil(pane.lo / step) * step; t <= pane.hi; t += step)
            ticks.append(t);
    }
    for (double t : std::as_const(ticks)) {
        const qreal y = yOf(pane, t);
        if (y < pane.rect.top() - 1 || y > pane.rect.bottom() + 1)
            continue;
        const bool guide = (pane.kind == PaneKind::Rsi && (t == 30.0 || t == 70.0))
            || (pane.kind == PaneKind::Stoch && (t == 20.0 || t == 80.0));
        p.setPen(QPen(guide ? Theme::dimText(pal) : gridColor, 1, guide ? Qt::DashLine : Qt::SolidLine));
        p.drawLine(QPointF(pane.rect.left(), y), QPointF(pane.rect.right(), y));

        QString label;
        switch (pane.kind) {
        case PaneKind::Price: label = fmtPrice(t); break;
        case PaneKind::Volume: label = fmtVolume(t); break;
        case PaneKind::Macd:
            // 값의 폭이 크면 정수로 (0.00 대신 0)
            label = (pane.hi - pane.lo) >= 20 ? krLocale().toString(t, 'f', 0) : fmtIndicator(t);
            break;
        default: label = QString::number(int(t)); break;
        }
        p.setPen(axisText);
        p.drawText(QRectF(pane.rect.right() + 6, y - fm.height() / 2.0, kAxisW - 8, fm.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, label);
    }

    // 내용
    p.save();
    p.setClipRect(pane.rect.adjusted(0, -2, 0, 2));
    switch (pane.kind) {
    case PaneKind::Price:
        drawPriceContent(p, pane, first, last);
        break;
    case PaneKind::Volume:
        drawVolumeContent(p, pane, first, last);
        break;
    case PaneKind::Macd:
        drawMacdContent(p, pane, first, last);
        break;
    case PaneKind::Rsi:
        drawLineSeries(p, pane, m_rsi, first, last, kRsiColor, 1.5);
        break;
    case PaneKind::Stoch:
        drawLineSeries(p, pane, m_stK, first, last, kStochK, 1.4);
        drawLineSeries(p, pane, m_stD, first, last, kStochD, 1.4);
        break;
    }
    p.restore();

    // 마지막 가격 표시 (가격 영역 오른쪽 축에 꼬리표)
    if (pane.kind == PaneKind::Price) {
        const Candle &lc = m_c.last();
        const qreal y = yOf(pane, lc.close);
        if (y >= pane.rect.top() && y <= pane.rect.bottom()) {
            const int dir = (m_c.size() > 1) ? (lc.close > m_c[m_c.size() - 2].close ? 1
                                                : lc.close < m_c[m_c.size() - 2].close ? -1 : 0)
                                             : candleDirection(int(m_c.size()) - 1);
            QColor c = Theme::directionColor(pal, dir == 0 ? 1 : dir);
            if (dir == 0)
                c = Theme::dimText(pal);
            QColor lineColor = c;
            lineColor.setAlpha(150);
            p.setPen(QPen(lineColor, 1, Qt::DashLine));
            p.drawLine(QPointF(pane.rect.left(), y), QPointF(pane.rect.right(), y));
            const QString t = fmtPrice(lc.close);
            const QRectF tag(pane.rect.right() + 2, y - fm.height() / 2.0 - 2, kAxisW - 4, fm.height() + 4);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(tag, 3, 3);
            p.setPen(Qt::white);
            p.drawText(tag.adjusted(4, 0, -2, 0), Qt::AlignLeft | Qt::AlignVCenter, t);
        }
    }

    const int legendIndex = (m_hover >= 0) ? m_hover : last - 1;
    drawLegend(p, pane, legendIndex);
}

void ChartWidget::drawPriceContent(QPainter &p, const Pane &pane, int first, int last) const
{
    const QPalette pal = palette();
    const double w = slotWidth();
    const double bodyW = qMax(1.0, w * 0.68);

    // 볼린저 밴드 (영역 채우기 + 선)
    if (m_showBB) {
        QPolygonF upper, lower;
        for (int i = first; i < last; ++i) {
            if (qIsNaN(m_bbUp[i]))
                continue;
            upper << QPointF(xOf(i), yOf(pane, m_bbUp[i]));
            lower.prepend(QPointF(xOf(i), yOf(pane, m_bbDn[i])));
        }
        if (upper.size() > 1) {
            QColor fill = kBollinger;
            fill.setAlpha(28);
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            QPolygonF band = upper;
            band += lower;
            p.drawPolygon(band);
        }
        drawLineSeries(p, pane, m_bbUp, first, last, kBollinger, 1.0);
        drawLineSeries(p, pane, m_bbDn, first, last, kBollinger, 1.0);
        drawLineSeries(p, pane, m_bbMid, first, last, kBollinger.darker(115), 1.0);
    }

    // 캔들
    p.setRenderHint(QPainter::Antialiasing, false);
    for (int i = first; i < last; ++i) {
        const Candle &c = m_c[i];
        const int dir = candleDirection(i);
        const QColor col = dir == 0 ? pal.color(QPalette::Text) : Theme::directionColor(pal, dir);
        const qreal x = xOf(i);
        const qreal yh = yOf(pane, c.high), yl = yOf(pane, c.low);
        const qreal yo = yOf(pane, c.open), yc = yOf(pane, c.close);
        p.setPen(QPen(col, 1));
        p.drawLine(QPointF(x, yh), QPointF(x, yl));
        if (bodyW >= 2.0) {
            const qreal top = qMin(yo, yc);
            const qreal h = qMax<qreal>(1.0, qAbs(yc - yo));
            p.fillRect(QRectF(x - bodyW / 2.0, top, bodyW, h), col);
        }
    }
    p.setRenderHint(QPainter::Antialiasing, true);

    // 이동평균선
    if (m_showMA) {
        for (int k = 3; k >= 0; --k)
            drawLineSeries(p, pane, m_ma[k], first, last, maColor(k), k == 0 ? 1.2 : 1.4);
    }

    // 보이는 구간의 최고가/최저가 표시
    int hiIdx = first, loIdx = first;
    for (int i = first; i < last; ++i) {
        if (m_c[i].high > m_c[hiIdx].high)
            hiIdx = i;
        if (m_c[i].low < m_c[loIdx].low)
            loIdx = i;
    }
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.85);
    p.setFont(small);
    const QFontMetrics fm(small);
    auto mark = [&](int idx, double value, bool isHigh) {
        const QString t = (isHigh ? tr("최고 ") : tr("최저 ")) + fmtPrice(value)
            + QStringLiteral(" (") + fmtDate(m_c[idx].date, false) + QLatin1Char(')');
        const qreal x = xOf(idx);
        const qreal y = yOf(pane, value);
        const bool leftSide = x > pane.rect.center().x();
        const qreal tw = fm.horizontalAdvance(t);
        const qreal tx = leftSide ? x - 14 - tw : x + 14;
        const qreal ty = isHigh ? qMax(pane.rect.top() + fm.ascent(), y - 2) : qMin(pane.rect.bottom() - 2, y + fm.ascent() + 2);
        const QColor col = Theme::directionColor(pal, isHigh ? 1 : -1);
        p.setPen(QPen(col, 1));
        p.drawLine(QPointF(x, y), QPointF(leftSide ? x - 12 : x + 12, ty - fm.ascent() / 2.0));
        p.drawText(QPointF(tx, ty), t);
    };
    mark(hiIdx, m_c[hiIdx].high, true);
    mark(loIdx, m_c[loIdx].low, false);
}

void ChartWidget::drawVolumeContent(QPainter &p, const Pane &pane, int first, int last) const
{
    const QPalette pal = palette();
    const double bodyW = qMax(1.0, slotWidth() * 0.68);
    p.setRenderHint(QPainter::Antialiasing, false);
    for (int i = first; i < last; ++i) {
        const int dir = candleDirection(i);
        QColor col = dir == 0 ? Theme::dimText(pal) : Theme::directionColor(pal, dir);
        col.setAlpha(190);
        const qreal y = yOf(pane, m_c[i].volume);
        p.fillRect(QRectF(xOf(i) - bodyW / 2.0, y, bodyW, pane.rect.bottom() - y), col);
    }
    p.setRenderHint(QPainter::Antialiasing, true);
}

void ChartWidget::drawMacdContent(QPainter &p, const Pane &pane, int first, int last) const
{
    const QPalette pal = palette();
    const qreal y0 = yOf(pane, 0.0);
    p.setPen(QPen(Theme::dimText(pal), 1));
    p.drawLine(QPointF(pane.rect.left(), y0), QPointF(pane.rect.right(), y0));

    // OSC 막대
    const double bodyW = qMax(1.0, slotWidth() * 0.6);
    p.setRenderHint(QPainter::Antialiasing, false);
    for (int i = first; i < last; ++i) {
        if (qIsNaN(m_osc[i]))
            continue;
        QColor col = Theme::directionColor(pal, m_osc[i] >= 0 ? 1 : -1);
        col.setAlpha(150);
        const qreal y = yOf(pane, m_osc[i]);
        p.fillRect(QRectF(xOf(i) - bodyW / 2.0, qMin(y, y0), bodyW, qAbs(y0 - y)), col);
    }
    p.setRenderHint(QPainter::Antialiasing, true);

    drawLineSeries(p, pane, m_macd, first, last, kMacdColor, 1.5);
    drawLineSeries(p, pane, m_signal, first, last, kSignalColor, 1.5);
}

void ChartWidget::drawLineSeries(QPainter &p, const Pane &pane, const QVector<double> &v, int first, int last,
                                 const QColor &color, qreal width) const
{
    QPolygonF poly;
    p.setPen(QPen(color, width));
    p.setBrush(Qt::NoBrush);
    for (int i = first; i < last; ++i) {
        if (qIsNaN(v[i])) {
            if (poly.size() > 1)
                p.drawPolyline(poly);
            poly.clear();
            continue;
        }
        poly << QPointF(xOf(i), yOf(pane, v[i]));
    }
    if (poly.size() > 1)
        p.drawPolyline(poly);
}

void ChartWidget::drawLegend(QPainter &p, const Pane &pane, int index) const
{
    if (index < 0 || index >= m_c.size())
        return;
    const QPalette pal = palette();
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() * 0.9);
    p.setFont(f);
    const QFontMetrics fm(f);

    struct Part { QString text; QColor color; };
    auto drawRow = [&](qreal y, const QVector<Part> &parts) {
        qreal x = pane.rect.left() + 6;
        for (const Part &part : parts) {
            p.setPen(part.color);
            p.drawText(QPointF(x, y), part.text);
            x += fm.horizontalAdvance(part.text) + 10;
        }
    };

    const Candle &c = m_c[index];
    const QColor text = pal.color(QPalette::Text);
    const QColor dim = Theme::dimText(pal);
    const double legend = (pane.kind == PaneKind::Price) ? kLegendH * 2 : kLegendH;
    const qreal row1 = pane.rect.top() - legend + fm.ascent() + 2;

    switch (pane.kind) {
    case PaneKind::Price: {
        const double prev = index > 0 ? m_c[index - 1].close : c.open;
        const double rate = prev > 0 ? (c.close - prev) / prev * 100.0 : 0.0;
        const int dir = rate > 0 ? 1 : rate < 0 ? -1 : 0;
        const QColor dc = dir == 0 ? text : Theme::directionColor(pal, dir);
        drawRow(row1, {{fmtDate(c.date, true), dim},
                       {tr("시 ") + fmtPrice(c.open), text},
                       {tr("고 ") + fmtPrice(c.high), text},
                       {tr("저 ") + fmtPrice(c.low), text},
                       {tr("종 ") + fmtPrice(c.close), dc},
                       {(rate > 0 ? QStringLiteral("+") : QString()) + QString::number(rate, 'f', 2) + QLatin1Char('%'), dc},
                       {tr("거래량 ") + krLocale().toString(c.volume, 'f', 0), dim}});
        QVector<Part> row2;
        if (m_showMA) {
            for (int k = 0; k < 4; ++k)
                row2.append({QStringLiteral("MA%1 ").arg(kMaPeriods[k]) + fmtPrice(m_ma[k][index]), maColor(k)});
        }
        if (m_showBB)
            row2.append({tr("BB(20,2) ") + fmtPrice(m_bbUp[index]) + QStringLiteral(" / ") + fmtPrice(m_bbDn[index]), kBollinger});
        if (!row2.isEmpty())
            drawRow(row1 + kLegendH, row2);
        break;
    }
    case PaneKind::Volume: {
        const int dir = candleDirection(index);
        drawRow(row1, {{tr("거래량"), dim},
                       {krLocale().toString(c.volume, 'f', 0), dir == 0 ? text : Theme::directionColor(pal, dir)}});
        break;
    }
    case PaneKind::Macd:
        drawRow(row1, {{QStringLiteral("MACD(12,26,9)"), dim},
                       {QStringLiteral("MACD ") + fmtIndicator(m_macd[index]), kMacdColor},
                       {tr("시그널 ") + fmtIndicator(m_signal[index]), kSignalColor},
                       {QStringLiteral("OSC ") + fmtIndicator(m_osc[index]),
                        qIsNaN(m_osc[index]) ? text : Theme::directionColor(pal, m_osc[index] >= 0 ? 1 : -1)}});
        break;
    case PaneKind::Rsi:
        drawRow(row1, {{QStringLiteral("RSI(14)"), dim}, {fmtIndicator(m_rsi[index]), kRsiColor}});
        break;
    case PaneKind::Stoch:
        drawRow(row1, {{tr("스토캐스틱 슬로우(12,5,5)"), dim},
                       {QStringLiteral("%K ") + fmtIndicator(m_stK[index]), kStochK},
                       {QStringLiteral("%D ") + fmtIndicator(m_stD[index]), kStochD}});
        break;
    }
}

void ChartWidget::drawDateAxis(QPainter &p, const QRectF &area, int first, int last) const
{
    const QPalette pal = palette();
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.86);
    p.setFont(small);
    const QFontMetrics fm(small);
    const double w = slotWidth();
    const int labelW = fm.horizontalAdvance(fmtDate(m_c[first].date, false)) + 24;
    const int step = qMax(1, int(std::ceil(labelW / w)));
    const qreal y = area.bottom() + 3;

    p.setPen(Theme::dimText(pal));
    for (int i = last - 1; i >= first; i -= step) {
        const qreal x = xOf(i);
        p.drawLine(QPointF(x, area.bottom()), QPointF(x, area.bottom() + 3));
        const QString t = fmtDate(m_c[i].date, false);
        const qreal tw = fm.horizontalAdvance(t);
        const qreal tx = qBound(area.left(), x - tw / 2.0, area.right() - tw);
        p.drawText(QPointF(tx, y + fm.ascent() + 1), t);
    }
}

void ChartWidget::drawCrosshair(QPainter &p, const QVector<Pane> &panes) const
{
    if (m_hover < 0 || m_hover >= m_c.size())
        return;
    const QPalette pal = palette();
    const QRectF plot = plotArea();
    const QColor lineColor = Theme::isDark(pal) ? QColor(255, 255, 255, 110) : QColor(0, 0, 0, 100);
    const qreal x = xOf(m_hover);

    p.setPen(QPen(lineColor, 1, Qt::DashLine));
    p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));

    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.86);
    p.setFont(small);
    const QFontMetrics fm(small);
    const QColor tagBg = Theme::isDark(pal) ? QColor(0x55, 0x5a, 0x63) : QColor(0x50, 0x55, 0x5e);

    // 날짜 꼬리표
    const QString dt = fmtDate(m_c[m_hover].date, true);
    const qreal tw = fm.horizontalAdvance(dt) + 10;
    QRectF dateTag(qBound(plot.left(), x - tw / 2.0, plot.right() - tw), plot.bottom() + 2, tw, fm.height() + 2);
    p.setPen(Qt::NoPen);
    p.setBrush(tagBg);
    p.drawRoundedRect(dateTag, 3, 3);
    p.setPen(Qt::white);
    p.drawText(dateTag, Qt::AlignCenter, dt);

    // 가로선 + 값 꼬리표 (마우스가 있는 영역)
    for (const Pane &pane : panes) {
        if (!pane.rect.contains(QPointF(pane.rect.center().x(), m_mouse.y())))
            continue;
        const qreal y = m_mouse.y();
        p.setPen(QPen(lineColor, 1, Qt::DashLine));
        p.drawLine(QPointF(pane.rect.left(), y), QPointF(pane.rect.right(), y));
        const double v = pane.lo + (pane.rect.bottom() - y) / pane.rect.height() * (pane.hi - pane.lo);
        QString t;
        switch (pane.kind) {
        case PaneKind::Price: t = fmtPrice(v); break;
        case PaneKind::Volume: t = fmtVolume(v); break;
        default: t = fmtIndicator(v); break;
        }
        const QRectF tag(pane.rect.right() + 2, y - fm.height() / 2.0 - 2, kAxisW - 4, fm.height() + 4);
        p.setPen(Qt::NoPen);
        p.setBrush(tagBg);
        p.drawRoundedRect(tag, 3, 3);
        p.setPen(Qt::white);
        p.drawText(tag.adjusted(4, 0, -2, 0), Qt::AlignLeft | Qt::AlignVCenter, t);
        break;
    }
}

// ── 마우스 ──

void ChartWidget::mouseMoveEvent(QMouseEvent *e)
{
    m_mouse = e->position();
    if (m_dragging && !m_c.isEmpty()) {
        const int shift = qRound((m_dragX - m_mouse.x()) / slotWidth());
        m_end = m_dragEnd + shift;
        clampView();
    }
    m_hover = indexAt(m_mouse.x());
    update();
}

void ChartWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragX = e->position().x();
        m_dragEnd = m_end;
        setCursor(Qt::ClosedHandCursor);
    }
}

void ChartWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::CrossCursor);
    }
}

void ChartWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    resetView();
    update();
}

void ChartWidget::wheelEvent(QWheelEvent *e)
{
    if (m_c.isEmpty())
        return;
    const QRectF plot = plotArea();
    const double steps = e->angleDelta().y() / 120.0;
    if (steps == 0)
        return;
    const double frac = qBound(0.0, (e->position().x() - plot.left()) / plot.width(), 1.0);
    const double anchor = firstVisible() + frac * m_visible;
    const int newVisible = qMax(1, int(std::round(m_visible * std::pow(0.85, steps))));
    m_visible = newVisible == m_visible ? m_visible + (steps > 0 ? -1 : 1) : newVisible;
    clampView();
    const int newFirst = int(std::round(anchor - frac * m_visible));
    m_end = newFirst + m_visible;
    clampView();
    m_hover = indexAt(e->position().x());
    update();
}

void ChartWidget::leaveEvent(QEvent *)
{
    m_hover = -1;
    update();
}
