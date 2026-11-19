#include "QuoteListModel.h"
#include "Theme.h"

namespace {
QString signedPercent(double v)
{
    return (v > 0 ? QStringLiteral("+") : QString()) + QString::number(v, 'f', 2) + QLatin1Char('%');
}
} // namespace

QuoteListModel::QuoteListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void QuoteListModel::setEntries(const QList<StockEntry> &entries)
{
    QHash<QString, Row> old;
    for (const Row &r : std::as_const(m_rows))
        old.insert(r.entry.code, r);

    beginResetModel();
    m_rows.clear();
    for (const StockEntry &e : entries) {
        Row r = old.value(e.code);
        r.entry = e;
        m_rows.append(r);
    }
    endResetModel();
    emit countChanged();
}

bool QuoteListModel::updateQuote(const Quote &q)
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].entry.code != q.code)
            continue;
        m_rows[i].quote = q;
        m_rows[i].hasQuote = true;
        if (!q.name.isEmpty())
            m_rows[i].entry.name = q.name;
        const QModelIndex idx = index(i);
        emit dataChanged(idx, idx);
        return true;
    }
    return false;
}

bool QuoteListModel::contains(const QString &code) const
{
    for (const Row &r : m_rows)
        if (r.entry.code == code)
            return true;
    return false;
}

int QuoteListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant QuoteListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows[index.row()];
    const Quote &q = r.quote;
    switch (role) {
    case CodeRole: return r.entry.code;
    case NameRole: return r.entry.name.isEmpty() ? r.entry.code : r.entry.name;
    case MarketRole: return r.entry.market;
    case HasQuoteRole: return r.hasQuote;
    case PriceRole: return r.hasQuote ? q.price : QStringLiteral("-");
    case ChangeTextRole: return r.hasQuote ? Theme::arrowText(q.direction, q.change) : QString();
    case RateTextRole: return r.hasQuote ? signedPercent(q.rate) : QString();
    case DirectionRole: return q.direction;
    case HasHighLowRole: return r.hasQuote && q.hasHighLow;
    case HighRole: return q.high;
    case HighRateTextRole: return signedPercent(q.highRate);
    case HighDirRole: return q.highDirection;
    case LowRole: return q.low;
    case LowRateTextRole: return signedPercent(q.lowRate);
    case LowDirRole: return q.lowDirection;
    case VolumeRole: return q.volume;
    case TradeValueRole: return q.tradeValue;
    }
    return {};
}

QHash<int, QByteArray> QuoteListModel::roleNames() const
{
    return {
        {CodeRole, "code"},
        {NameRole, "name"},
        {MarketRole, "market"},
        {HasQuoteRole, "hasQuote"},
        {PriceRole, "price"},
        {ChangeTextRole, "changeText"},
        {RateTextRole, "rateText"},
        {DirectionRole, "direction"},
        {HasHighLowRole, "hasHighLow"},
        {HighRole, "high"},
        {HighRateTextRole, "highRateText"},
        {HighDirRole, "highDir"},
        {LowRole, "low"},
        {LowRateTextRole, "lowRateText"},
        {LowDirRole, "lowDir"},
        {VolumeRole, "volume"},
        {TradeValueRole, "tradeValue"},
    };
}
