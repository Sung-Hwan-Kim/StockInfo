#include "RefreshButton.h"
#include "Theme.h"

#include <QEnterEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QtMath>

namespace {
constexpr int kIcon = 14;  // ⟳ 아이콘 크기
constexpr int kPadX = 10;  // 좌우 여백
constexpr int kGap = 6;    // 아이콘 ~ 글자 간격

// 원형 화살표(⟳) 그리기: 중심 c, 반지름 r, 회전 angle(도)
void drawRefreshGlyph(QPainter &p, const QPointF &c, qreal r, qreal angle, const QColor &color)
{
    p.save();
    p.translate(c);
    p.rotate(angle);

    QPen pen(color, qMax<qreal>(1.6, r * 0.32), Qt::SolidLine, Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QRectF box(-r, -r, 2 * r, 2 * r);
    // 위쪽에서 시작해 시계 방향으로 300도 (Qt 각도: 1/16도, 반시계가 +)
    p.drawArc(box, 60 * 16, -300 * 16);

    // 화살촉: 호의 끝(위쪽, 60도 지점)에 시계 방향으로
    const qreal a = qDegreesToRadians(60.0);
    const QPointF tip(r * qCos(a), -r * qSin(a));
    const qreal s = r * 0.75;
    QPainterPath head;
    head.moveTo(tip + QPointF(s * 0.95, s * 0.05));
    head.lineTo(tip + QPointF(-s * 0.15, -s * 0.75));
    head.lineTo(tip + QPointF(-s * 0.35, s * 0.55));
    head.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawPath(head);
    p.restore();
}
} // namespace

RefreshButton::RefreshButton(QWidget *parent)
    : QToolButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::TabFocus);

    m_spin = new QVariantAnimation(this);
    m_spin->setStartValue(0.0);
    m_spin->setEndValue(360.0);
    m_spin->setDuration(650);
    connect(m_spin, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_angle = v.toReal();
        update();
    });
    connect(m_spin, &QVariantAnimation::finished, this, [this] {
        m_angle = 0.0;
        update();
    });

    m_flash = new QVariantAnimation(this);
    m_flash->setStartValue(1.0);
    m_flash->setEndValue(0.0);
    m_flash->setDuration(900);
    m_flash->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_flash, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_glow = v.toReal();
        update();
    });
}

void RefreshButton::setBusy(bool busy)
{
    if (busy == m_busy)
        return;
    m_busy = busy;
    if (busy) {
        m_spin->setLoopCount(-1); // 끝날 때까지 계속 회전
        if (m_spin->state() != QAbstractAnimation::Running)
            m_spin->start();
    } else if (m_spin->state() == QAbstractAnimation::Running) {
        // 지금 돌던 한 바퀴를 마치고 멈춤 (응답이 빨라도 회전이 눈에 보이게)
        m_spin->setLoopCount(m_spin->currentLoop() + 1);
    }
}

void RefreshButton::flash(bool ok)
{
    m_flashOk = ok;
    m_flash->stop();
    m_flash->start();
}

QString RefreshButton::label() const
{
    return text().isEmpty() ? QStringLiteral("실시간 갱신") : text();
}

QSize RefreshButton::sizeHint() const
{
    QFont f = font();
    f.setBold(true);
    const QFontMetrics fm(f);
    const int w = kPadX + kIcon + kGap + fm.horizontalAdvance(label()) + kPadX;
    const int h = qMax(fm.height() + 8, 24);
    return {w, h};
}

void RefreshButton::enterEvent(QEnterEvent *e)
{
    m_hover = true;
    update();
    QToolButton::enterEvent(e);
}

void RefreshButton::leaveEvent(QEvent *e)
{
    m_hover = false;
    update();
    QToolButton::leaveEvent(e);
}

void RefreshButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPalette pal = palette();
    const bool dark = Theme::isDark(pal);
    const QColor accent = pal.color(QPalette::Link);
    const QColor failColor = Theme::directionColor(pal, 1);
    const QColor labelColor = pal.color(QPalette::ButtonText);

    // 알약 모양 배경
    const QRectF r = QRectF(rect()).adjusted(1.5, 2.5, -1.5, -2.5);
    QPainterPath path;
    path.addRoundedRect(r, r.height() / 2, r.height() / 2);

    QColor bg = pal.color(QPalette::Button);
    if (isDown())
        bg = dark ? bg.lighter(150) : bg.darker(112);
    else if (m_hover)
        bg = dark ? bg.lighter(130) : bg.darker(105);
    p.fillPath(path, bg);

    // 완료 반짝임
    if (m_glow > 0.0) {
        QColor g = m_flashOk ? accent : failColor;
        g.setAlphaF((dark ? 0.45 : 0.30) * m_glow);
        p.fillPath(path, g);
    }

    QColor border = dark ? pal.color(QPalette::Light) : pal.color(QPalette::Mid);
    if (m_hover || m_busy || m_glow > 0.0)
        border = accent;
    p.setPen(QPen(border, 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    // ⟳ 아이콘 (갱신 중이면 회전)
    const QPointF c(r.left() + kPadX + kIcon / 2.0, r.center().y());
    drawRefreshGlyph(p, c, kIcon / 2.0 - 1.0, m_angle, accent);

    // 글자
    QFont f = font();
    f.setBold(true);
    p.setFont(f);
    p.setPen(labelColor);
    const qreal textLeft = r.left() + kPadX + kIcon + kGap;
    p.drawText(QRectF(textLeft, r.top(), r.right() - textLeft, r.height()),
               Qt::AlignLeft | Qt::AlignVCenter, label());

    // 키보드 포커스 표시
    if (hasFocus()) {
        p.setPen(QPen(pal.color(QPalette::Highlight), 1.0, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(2, 2, -2, -2), r.height() / 2 - 2, r.height() / 2 - 2);
    }
}
