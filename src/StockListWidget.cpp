#include "StockListWidget.h"
#include "Theme.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace {

constexpr int kCodeRole = Qt::UserRole + 1;
constexpr int kSubRole = Qt::UserRole + 10;    // 아래 줄 글자
constexpr int kDirRole = Qt::UserRole + 11;    // 위 줄 색 (1 상승 / -1 하락 / 0 보합, 없으면 기본색)
constexpr int kSubDirRole = Qt::UserRole + 12; // 아래 줄 색 (없으면 흐린 글자색)
constexpr int kSubCompactRole = Qt::UserRole + 13; // 폭이 좁을 때 쓰는 짧은 아래 줄 (현재가: 등락률만)
const char kCompactProperty[] = "compactMode";

constexpr qreal kSubScale = 0.84; // 아래 줄 글자 크기 비율

QString signedPercent(double v)
{
    return (v > 0 ? QStringLiteral("+") : QString()) + QString::number(v, 'f', 2) + QLatin1Char('%');
}

// 두 줄짜리 셀을 직접 그리는 델리게이트.
// (Qt 6.7 windows11 등 일부 스타일은 ForegroundRole 색을 무시하므로 글자는 모두 직접 그린다)
class QuoteDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem o(option);
        initStyleOption(&o, index);
        const QString top = o.text;
        const QString sub = subText(o, index);
        o.text.clear();

        const QWidget *w = o.widget;
        QStyle *style = w ? w->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &o, p, w); // 배경/선택 표시만

        const bool selected = o.state & QStyle::State_Selected;
        const QColor topColor = colorFor(o, index.data(kDirRole), selected, false);
        const QColor subColor = colorFor(o, index.data(kSubDirRole), selected, true);

        const QFont topFont = o.font;
        const QFont subFont = subFontOf(o.font);
        const QFontMetrics fmTop(topFont), fmSub(subFont);

        const int margin = textMargin(o);
        const QRect r = o.rect.adjusted(margin, 0, -margin, 0);
        const int total = fmTop.height() + fmSub.height();
        const int y = r.top() + (r.height() - total) / 2;
        const QRect topRect(r.left(), y, r.width(), fmTop.height());
        const QRect subRect(r.left(), y + fmTop.height(), r.width(), fmSub.height());
        const int hAlign = int(o.displayAlignment & Qt::AlignHorizontal_Mask);

        p->save();
        p->setFont(topFont);
        p->setPen(topColor);
        p->drawText(topRect, hAlign | Qt::AlignVCenter, fmTop.elidedText(top, Qt::ElideRight, r.width()));
        if (!sub.isEmpty()) {
            p->setFont(subFont);
            p->setPen(subColor);
            p->drawText(subRect, hAlign | Qt::AlignVCenter, fmSub.elidedText(sub, Qt::ElideRight, r.width()));
        }
        p->restore();
    }

    // 두 줄 중 넓은 쪽에 맞춘 크기 (열 자동 너비가 잘리지 않도록)
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem o(option);
        initStyleOption(&o, index);
        const QFontMetrics fmTop(o.font);
        const QFontMetrics fmSub(subFontOf(o.font));
        const QString sub = subText(o, index);
        const int w = qMax(fmTop.horizontalAdvance(o.text), fmSub.horizontalAdvance(sub)) + 2 * textMargin(o) + 2;
        const int h = fmTop.height() + fmSub.height() + 8;
        return {w, h};
    }

private:
    static QString subText(const QStyleOptionViewItem &o, const QModelIndex &index)
    {
        const bool compact = o.widget && o.widget->property(kCompactProperty).toBool();
        const QVariant shortText = index.data(kSubCompactRole);
        if (compact && shortText.isValid())
            return shortText.toString();
        return index.data(kSubRole).toString();
    }

    static QFont subFontOf(QFont f)
    {
        f.setBold(false);
        f.setPointSizeF(f.pointSizeF() * kSubScale);
        return f;
    }

    static QColor colorFor(const QStyleOptionViewItem &o, const QVariant &dir, bool selected, bool sub)
    {
        if (selected)
            return o.palette.color(QPalette::HighlightedText);
        if (dir.isValid())
            return Theme::directionColor(o.palette, dir.toInt());
        return sub ? Theme::dimText(o.palette) : o.palette.color(QPalette::Text);
    }

    static int textMargin(const QStyleOptionViewItem &o)
    {
        const QWidget *w = o.widget;
        QStyle *style = w ? w->style() : QApplication::style();
        return style->pixelMetric(QStyle::PM_FocusFrameHMargin, &o, w) + 2;
    }
};

} // namespace

// ── ListFrame ──

ListFrame::ListFrame(QWidget *content, QWidget *parent)
    : QWidget(parent)
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(3, 3, 3, 3);
    lay->setSpacing(0);
    lay->addWidget(content);
}

void ListFrame::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(r, 4, 4);
    p.fillPath(path, palette().color(QPalette::Base));
    p.setPen(QPen(QColor(0x8a, 0x8a, 0x8a), 2)); // 그림의 회색 리스트박스 테두리
    p.drawPath(path);
}

// ── StockListWidget ──

StockListWidget::StockListWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setObjectName(QStringLiteral("stockList"));
    setColumnCount(ColCount);
    setHeaderLabels({tr("종목명"), tr("현재가"), tr("고가"), tr("저가"), tr("거래량")});
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAllColumnsShowFocus(true);
    setSortingEnabled(false);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setTextElideMode(Qt::ElideRight);
    setItemDelegate(new QuoteDelegate(this));

    QHeaderView *hdr = header();
    hdr->setStretchLastSection(false);
    hdr->setSectionsMovable(false);
    hdr->setSectionResizeMode(ColName, QHeaderView::Stretch);
    for (int c = ColPrice; c < ColCount; ++c) {
        hdr->setSectionResizeMode(c, QHeaderView::ResizeToContents);
        headerItem()->setTextAlignment(c, Qt::AlignRight | Qt::AlignVCenter);
    }
    hdr->setMinimumSectionSize(40);

    // 테두리는 ListFrame이 그린다 (스타일시트는 앱 팔레트/스킨 전환과 충돌하므로 사용하지 않음)
    setFrameShape(QFrame::NoFrame);

    connect(this, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        if (item)
            emit openRequested(item->data(ColName, kCodeRole).toString());
    });
}

void StockListWidget::setStocks(const QList<StockEntry> &stocks)
{
    // 기존 항목의 시세 표시는 유지 (코드가 같은 항목 재사용)
    QHash<QString, QTreeWidgetItem *> old = m_items;
    m_items.clear();

    // 트리에서 모두 떼어낸 다음 순서대로 다시 넣는다
    while (topLevelItemCount() > 0)
        takeTopLevelItem(0);

    for (const StockEntry &e : stocks) {
        QTreeWidgetItem *item = old.take(e.code);
        if (!item) {
            item = new QTreeWidgetItem;
            item->setData(ColName, kCodeRole, e.code);
            item->setText(ColName, e.name.isEmpty() ? e.code : e.name);
            item->setData(ColName, kSubRole, e.code);
            for (int c = ColPrice; c < ColCount; ++c) {
                item->setTextAlignment(c, Qt::AlignRight | Qt::AlignVCenter);
                item->setText(c, QStringLiteral("-"));
            }
            QFont bold = font();
            bold.setBold(true);
            item->setFont(ColPrice, bold);
            item->setToolTip(ColName, e.code);
        }
        addTopLevelItem(item);
        m_items.insert(e.code, item);
    }
    qDeleteAll(old);
    updateColumnVisibility();
}

void StockListWidget::updateQuote(const Quote &q)
{
    QTreeWidgetItem *item = m_items.value(q.code);
    if (!item)
        return;

    // 종목명 / 종목코드
    item->setText(ColName, q.name.isEmpty() ? q.code : q.name);
    item->setData(ColName, kSubRole, q.code);

    // 현재가 / 대비 + 등락률
    item->setText(ColPrice, q.price);
    item->setData(ColPrice, kDirRole, q.direction);
    item->setData(ColPrice, kSubRole,
                  Theme::arrowText(q.direction, q.change) + QStringLiteral("  ") + signedPercent(q.rate));
    item->setData(ColPrice, kSubDirRole, q.direction);
    item->setData(ColPrice, kSubCompactRole, signedPercent(q.rate));

    // 고가 / 저가 (전일 종가 대비 %)
    if (q.hasHighLow) {
        item->setText(ColHigh, q.high);
        item->setData(ColHigh, kDirRole, q.highDirection);
        item->setData(ColHigh, kSubRole, signedPercent(q.highRate));
        item->setData(ColHigh, kSubDirRole, q.highDirection);

        item->setText(ColLow, q.low);
        item->setData(ColLow, kDirRole, q.lowDirection);
        item->setData(ColLow, kSubRole, signedPercent(q.lowRate));
        item->setData(ColLow, kSubDirRole, q.lowDirection);
    } else {
        for (int c : {ColHigh, ColLow}) {
            item->setText(c, QStringLiteral("-"));
            item->setData(c, kDirRole, QVariant());
            item->setData(c, kSubRole, QString());
            item->setData(c, kSubDirRole, QVariant());
        }
    }

    // 거래량 / 거래대금
    item->setText(ColVolume, q.volume);
    item->setData(ColVolume, kSubRole, q.tradeValue);

    const QString highText = q.hasHighLow ? QStringLiteral("%1 (%2)").arg(q.high, signedPercent(q.highRate)) : q.high;
    const QString lowText = q.hasHighLow ? QStringLiteral("%1 (%2)").arg(q.low, signedPercent(q.lowRate)) : q.low;
    const QString tip = tr("%1 (%2)\n현재가 %3  %4 (%5)\n시가 %6\n고가 %7\n저가 %8\n거래량 %9  거래대금 %10\n%11 %12")
                            .arg(q.name, q.code, q.price, Theme::arrowText(q.direction, q.change),
                                 signedPercent(q.rate), q.open, highText, lowText, q.volume)
                            .arg(q.tradeValue, StockService::marketStatusText(q.marketStatus),
                                 q.tradedAt.toString(QStringLiteral("MM/dd HH:mm:ss")));
    for (int c = 0; c < ColCount; ++c)
        item->setToolTip(c, tip);

    updateColumnVisibility();
}

void StockListWidget::recolor()
{
    viewport()->update();
}

void StockListWidget::setListFontSize(int pt)
{
    QFont f = font();
    f.setPointSize(pt);
    setFont(f);
    header()->setFont(f);
    // 현재가(굵게) 글꼴도 새 크기로
    QFont bold = f;
    bold.setBold(true);
    for (QTreeWidgetItem *item : std::as_const(m_items))
        item->setFont(ColPrice, bold);
    updateColumnVisibility();
}

void StockListWidget::resizeEvent(QResizeEvent *e)
{
    QTreeWidget::resizeEvent(e);
    updateColumnVisibility();
}

int StockListWidget::measureColumn(int col, bool compactSub) const
{
    QFont topFont = font();
    if (col == ColPrice)
        topFont.setBold(true);
    QFont subFont = font();
    subFont.setPointSizeF(subFont.pointSizeF() * kSubScale);
    const QFontMetrics fmTop(topFont), fmSub(subFont);
    const int pad = 2 * (style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, this) + 2) + 2;

    int w = header()->sectionSizeHint(col);
    for (const QTreeWidgetItem *item : std::as_const(m_items)) {
        QString sub = item->data(col, kSubRole).toString();
        const QVariant shortText = item->data(col, kSubCompactRole);
        if (compactSub && shortText.isValid())
            sub = shortText.toString();
        w = qMax(w, qMax(fmTop.horizontalAdvance(item->text(col)), fmSub.horizontalAdvance(sub)) + pad);
    }
    return w;
}

// 폭에 따라 표시 방법을 조절해서 종목명/현재가/등락률/고가/저가는 항상 보이게 한다
//   넓음: 거래량 열 표시, 현재가 아래 줄 = "▲ 대비  등락률%"
//   보통: 거래량 열 숨김
//   좁음: 현재가 아래 줄을 "등락률%"만 표시 (대비 금액은 툴팁)
StockListWidget::Fit StockListWidget::computeFit() const
{
    const QFontMetrics fm(font());
    const int avail = viewport()->width();
    const int nameMin = fm.horizontalAdvance(QStringLiteral("가나다라마")) + 12; // 한글 5글자
    const int nameWant = qMin(measureColumn(ColName, false), nameMin);
    const int highLow = measureColumn(ColHigh, false) + measureColumn(ColLow, false);
    const int fullCore = nameWant + measureColumn(ColPrice, false) + highLow;

    Fit fit;
    fit.full = avail >= fullCore;
    fit.volume = fit.full && avail >= fullCore + measureColumn(ColVolume, false);
    return fit;
}

void StockListWidget::updateColumnVisibility()
{
    // 두 목록 중 하나라도 좁으면 둘 다 같은 방식으로 표시 (코스피/코스닥 모양 통일)
    const Fit own = computeFit();
    Fit fit = own;
    if (m_peer) {
        const Fit other = m_peer->computeFit();
        fit.full = own.full && other.full;
        fit.volume = own.volume && other.volume;
    }
    const bool compact = !fit.full;
    const bool showVolume = fit.volume;

    bool changed = false;
    if (property(kCompactProperty).toBool() != compact) {
        setProperty(kCompactProperty, compact);
        changed = true;
    }
    if (isColumnHidden(ColVolume) == showVolume) {
        setColumnHidden(ColVolume, !showVolume);
        changed = true;
    }
    if (changed) {
        doItemsLayout(); // 열 자동 너비 다시 계산
        viewport()->update();
    }

    if (m_peer && !m_syncing) {
        m_peer->m_syncing = true;
        m_peer->updateColumnVisibility();
        m_peer->m_syncing = false;
    }
}

void StockListWidget::contextMenuEvent(QContextMenuEvent *e)
{
    // QAbstractScrollArea의 contextMenuEvent 좌표는 viewport 기준
    QTreeWidgetItem *item = itemAt(e->pos());
    QMenu menu(this);
    if (item) {
        setCurrentItem(item);
        const QString code = item->data(ColName, kCodeRole).toString();
        const QString name = item->text(ColName);
        menu.addAction(tr("네이버 금융에서 보기"), this, [this, code] { emit openRequested(code); });
        menu.addAction(tr("'%1' 삭제").arg(name), this, [this, code] { emit removeRequested(code); });
        menu.addSeparator();
    }
    menu.addAction(tr("종목 추가/관리..."), this, [this] { emit manageRequested(); });
    menu.exec(e->globalPos());
}
