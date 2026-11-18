#include "IndexPanel.h"
#include "Theme.h"

#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>

namespace {

// rect 안에 text가 들어가도록 픽셀 크기를 맞춘 폰트
QFont fittedFont(QFont base, const QString &text, const QRectF &rect, qreal maxPx, bool bold)
{
    base.setBold(bold);
    int px = qMax(6, int(qMin(maxPx, rect.height() * 0.82)));
    base.setPixelSize(px);
    if (text.isEmpty())
        return base;
    while (px > 6) {
        QFontMetrics fm(base);
        if (fm.horizontalAdvance(text) <= rect.width())
            break;
        px = qMax(6, int(px * 0.92));
        base.setPixelSize(px);
    }
    return base;
}

void drawLine(QPainter &p, const QFont &base, const QString &text, const QRectF &rect,
              const QColor &color, qreal maxPx, bool bold)
{
    p.setFont(fittedFont(base, text, rect, maxPx, bold));
    p.setPen(color);
    p.drawText(rect, Qt::AlignCenter, text);
}

} // namespace

IndexPanel::IndexPanel(const QString &code, const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_code(code)
    , m_title(title)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setToolTip(tr("더블클릭하면 %1 지수 상세 창(차트)을 엽니다.\n오른쪽 클릭: 네이버 금융에서 보기").arg(title));
}

void IndexPanel::setQuote(const Quote &q)
{
    m_quote = q;
    m_hasQuote = true;
    m_stale = false;
    update();
}

void IndexPanel::setStale(bool stale)
{
    if (m_stale != stale) {
        m_stale = stale;
        update();
    }
}

void IndexPanel::mouseDoubleClickEvent(QMouseEvent *)
{
    emit detailRequested(m_code);
}

void IndexPanel::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu(this);
    QAction *detail = menu.addAction(tr("상세 보기 (차트)"), this, [this] { emit detailRequested(m_code); });
    QFont bold = detail->font();
    bold.setBold(true);
    detail->setFont(bold);
    menu.addAction(tr("네이버 금융에서 보기"), this, [this] { emit openRequested(m_code); });
    menu.exec(e->globalPos());
}

void IndexPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QPalette pal = palette();
    const QColor textColor = pal.color(QPalette::WindowText);
    const QColor dimColor = Theme::dimText(pal);

    // 배경 + 굵은 테두리
    const qreal border = 3.0;
    const QRectF frame = QRectF(rect()).adjusted(border / 2, border / 2, -border / 2, -border / 2);
    QPainterPath path;
    path.addRoundedRect(frame, 6, 6);
    p.fillPath(path, pal.color(QPalette::Base));
    p.setPen(QPen(textColor, border));
    p.drawPath(path);

    const QRectF inner = frame.adjusted(10, 6, -10, -6);
    const qreal h = inner.height();
    const QFont base = font();

    // 세로 배치 비율: 제목 / 지수 / 등락 / 시고저 / 상태
    const qreal fr[] = {0.17, 0.38, 0.19, 0.13, 0.13};
    QRectF rows[5];
    qreal y = inner.top();
    for (int i = 0; i < 5; ++i) {
        rows[i] = QRectF(inner.left(), y, inner.width(), h * fr[i]);
        y += h * fr[i];
    }

    drawLine(p, base, m_title, rows[0], textColor, 40, true);

    if (!m_hasQuote) {
        drawLine(p, base, tr("불러오는 중…"), rows[1].adjusted(0, 0, 0, rows[2].height()), dimColor, 28, false);
        return;
    }

    const QColor dirColor = Theme::directionColor(pal, m_quote.direction);

    drawLine(p, base, m_quote.price, rows[1], dirColor, 120, true);

    const QString changeText = QStringLiteral("%1   %2%3%")
                                   .arg(Theme::arrowText(m_quote.direction, m_quote.change),
                                        m_quote.rate > 0 ? QStringLiteral("+") : QString(),
                                        QString::number(m_quote.rate, 'f', 2));
    drawLine(p, base, changeText, rows[2], dirColor, 40, true);

    const QString detail = tr("시가 %1   고가 %2   저가 %3").arg(m_quote.open, m_quote.high, m_quote.low);
    drawLine(p, base, detail, rows[3], dimColor, 18, false);

    QString status = StockService::marketStatusText(m_quote.marketStatus);
    if (m_quote.tradedAt.isValid())
        status += QStringLiteral("  ·  ") + m_quote.tradedAt.toString(QStringLiteral("MM/dd HH:mm:ss"));
    if (!m_quote.tradeValue.isEmpty())
        status += tr("  ·  거래대금 %1").arg(m_quote.tradeValue);
    if (m_stale)
        status += tr("  (갱신 실패)");
    drawLine(p, base, status, rows[4], m_stale ? Theme::directionColor(pal, 1) : dimColor, 16, false);
}
