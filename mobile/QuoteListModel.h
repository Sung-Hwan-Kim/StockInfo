#pragma once

#include "AppSettings.h"
#include "StockService.h"

#include <QAbstractListModel>

// 모바일 종목 목록 (코스피 / 코스닥 각각 하나씩)
class QuoteListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles {
        CodeRole = Qt::UserRole + 1,
        NameRole,
        MarketRole,
        HasQuoteRole,
        PriceRole,
        ChangeTextRole, // "▲ 1,200"
        RateTextRole,   // "+1.23%"
        DirectionRole,  // 1 / -1 / 0
        HasHighLowRole,
        HighRole,
        HighRateTextRole,
        HighDirRole,
        LowRole,
        LowRateTextRole,
        LowDirRole,
        VolumeRole,
        TradeValueRole,
    };

    explicit QuoteListModel(QObject *parent = nullptr);

    void setEntries(const QList<StockEntry> &entries); // 이미 받은 시세는 유지
    bool updateQuote(const Quote &q);
    bool contains(const QString &code) const;

    int count() const { return int(m_rows.size()); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void countChanged();

private:
    struct Row
    {
        StockEntry entry;
        Quote quote;
        bool hasQuote = false;
    };
    QList<Row> m_rows;
};
