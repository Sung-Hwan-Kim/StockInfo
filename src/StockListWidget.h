#pragma once

#include "AppSettings.h"
#include "StockService.h"

#include <QHash>
#include <QTreeWidget>

// 종목 시세 리스트 (회색 테두리 리스트박스). 종목이 많으면 스크롤바가 생긴다.
class StockListWidget : public QTreeWidget
{
    Q_OBJECT
public:
    enum Column { ColName = 0, ColPrice, ColChange, ColRate, ColVolume, ColCount };

    explicit StockListWidget(QWidget *parent = nullptr);

    void setStocks(const QList<StockEntry> &stocks);
    bool hasCode(const QString &code) const { return m_items.contains(code); }
    void updateQuote(const Quote &q);
    void setListFontSize(int pt);

signals:
    void openRequested(const QString &code);
    void removeRequested(const QString &code);
    void manageRequested();

protected:
    void resizeEvent(QResizeEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    void updateColumnVisibility();

    QHash<QString, QTreeWidgetItem *> m_items;
};
