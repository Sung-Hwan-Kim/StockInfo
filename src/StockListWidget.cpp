#include "StockListWidget.h"
#include "Theme.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>

namespace {
constexpr int kCodeRole = Qt::UserRole + 1;

// 일부 스타일(Qt 6.7 windows11 등)이 ForegroundRole 색을 무시하므로 글자를 직접 그린다
class QuoteDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem o(option);
        initStyleOption(&o, index);
        const QString text = o.text;
        o.text.clear();

        const QWidget *w = o.widget;
        QStyle *style = w ? w->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &o, p, w); // 배경/선택 표시만

        const bool selected = o.state & QStyle::State_Selected;
        QColor color = o.palette.color(selected ? QPalette::HighlightedText : QPalette::Text);
        const QVariant fg = index.data(Qt::ForegroundRole);
        if (!selected && fg.isValid())
            color = fg.value<QBrush>().color();

        const int margin = textMargin(o);
        const QRect r = o.rect.adjusted(margin, 0, -margin, 0);
        p->save();
        p->setFont(o.font);
        p->setPen(color);
        const QString shown = QFontMetrics(o.font).elidedText(text, Qt::ElideRight, r.width());
        p->drawText(r, int(o.displayAlignment) | Qt::AlignVCenter, shown);
        p->restore();
    }

    // 직접 그리는 글자 폭에 맞춘 크기 (열 자동 너비가 잘리지 않도록)
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem o(option);
        initStyleOption(&o, index);
        const QSize base = QStyledItemDelegate::sizeHint(option, index);
        const QFontMetrics fm(o.font);
        const int w = fm.horizontalAdvance(o.text) + 2 * textMargin(o) + 2;
        const int h = qMax(base.height(), fm.height() + 6);
        return {qMax(base.width(), w), h};
    }

private:
    static int textMargin(const QStyleOptionViewItem &o)
    {
        const QWidget *w = o.widget;
        QStyle *style = w ? w->style() : QApplication::style();
        return style->pixelMetric(QStyle::PM_FocusFrameHMargin, &o, w) + 2;
    }
};
} // namespace

StockListWidget::StockListWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setObjectName(QStringLiteral("stockList"));
    setColumnCount(ColCount);
    setHeaderLabels({tr("종목명"), tr("현재가"), tr("대비"), tr("등락률"), tr("거래량")});
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAllColumnsShowFocus(true);
    setSortingEnabled(false);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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

    // 그림의 회색 리스트박스 테두리
    setStyleSheet(QStringLiteral(
        "QTreeWidget#stockList { border: 2px solid #8a8a8a; border-radius: 4px; }"));

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
            for (int c = ColPrice; c < ColCount; ++c) {
                item->setTextAlignment(c, Qt::AlignRight | Qt::AlignVCenter);
                item->setText(c, QStringLiteral("-"));
            }
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

    const QColor color = Theme::directionColor(palette(), q.direction);
    const QString sign = q.rate > 0 ? QStringLiteral("+") : QString();

    item->setText(ColName, q.name.isEmpty() ? q.code : q.name);
    item->setText(ColPrice, q.price);
    item->setText(ColChange, Theme::arrowText(q.direction, q.change));
    item->setText(ColRate, sign + QString::number(q.rate, 'f', 2) + QLatin1Char('%'));
    item->setText(ColVolume, q.volume);

    QFont bold = item->font(ColPrice);
    bold.setBold(true);
    item->setFont(ColPrice, bold);
    for (int c = ColPrice; c <= ColRate; ++c)
        item->setForeground(c, color);

    const QString tip = tr("%1 (%2)\n현재가 %3  %4 %5 (%6%7%)\n시가 %8  고가 %9  저가 %10\n거래량 %11  거래대금 %12\n%13 %14")
                            .arg(q.name, q.code, q.price, Theme::arrow(q.direction), q.change, sign,
                                 QString::number(q.rate, 'f', 2), q.open, q.high)
                            .arg(q.low, q.volume, q.tradeValue,
                                 StockService::marketStatusText(q.marketStatus),
                                 q.tradedAt.toString(QStringLiteral("MM/dd HH:mm:ss")));
    for (int c = 0; c < ColCount; ++c)
        item->setToolTip(c, tip);

    updateColumnVisibility();
}

void StockListWidget::setListFontSize(int pt)
{
    QFont f = font();
    f.setPointSize(pt);
    setFont(f);
    header()->setFont(f);
    // 기존 항목의 굵은 글꼴도 새 크기로
    for (QTreeWidgetItem *item : std::as_const(m_items)) {
        QFont b = f;
        b.setBold(true);
        item->setFont(ColPrice, b);
    }
    updateColumnVisibility();
}

void StockListWidget::resizeEvent(QResizeEvent *e)
{
    QTreeWidget::resizeEvent(e);
    updateColumnVisibility();
}

// 폭이 좁으면 거래량 → 대비 순서로 열을 숨겨서
// 종목명/현재가/등락률은 항상 읽을 수 있게 한다
void StockListWidget::updateColumnVisibility()
{
    auto colWidth = [this](int c) { return qMax(sizeHintForColumn(c), header()->sectionSizeHint(c)); };

    const QFontMetrics fm(font());
    // 종목명은 한글 5글자 정도는 보이도록 확보 (더 긴 이름은 … 으로 줄이고 툴팁에 전체 표시)
    const int nameWant = qMin(colWidth(ColName), fm.horizontalAdvance(QStringLiteral("가나다라마")) + 12);
    const int core = nameWant + colWidth(ColPrice) + colWidth(ColRate);
    const int avail = viewport()->width();

    const bool showChange = avail >= core + colWidth(ColChange);
    const bool showVolume = showChange && avail >= core + colWidth(ColChange) + colWidth(ColVolume);

    if (isColumnHidden(ColChange) == showChange)
        setColumnHidden(ColChange, !showChange);
    if (isColumnHidden(ColVolume) == showVolume)
        setColumnHidden(ColVolume, !showVolume);
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
