#pragma once

#include "AppSettings.h"
#include "StockService.h"

#include <QHash>
#include <QTreeWidget>

// 리스트를 감싸고 회색 둥근 테두리를 그리는 컨테이너
class ListFrame : public QWidget
{
    Q_OBJECT
public:
    explicit ListFrame(QWidget *content, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;
};

// 종목 시세 리스트 (회색 테두리 리스트박스). 종목이 많으면 스크롤바가 생긴다.
// 한 종목을 두 줄로 표시한다:
//   종목명   | 현재가            | 고가     | 저가     | 거래량
//   종목코드 | ▲ 대비  등락률%   | 고가 %   | 저가 %   | 거래대금
class StockListWidget : public QTreeWidget
{
    Q_OBJECT
public:
    enum Column { ColName = 0, ColPrice, ColHigh, ColLow, ColVolume, ColCount };

    explicit StockListWidget(QWidget *parent = nullptr);

    void setStocks(const QList<StockEntry> &stocks);
    bool hasCode(const QString &code) const { return m_items.contains(code); }
    void updateQuote(const Quote &q);
    void setListFontSize(int pt);
    // 현재 팔레트(스킨)에 맞춰 다시 그리기 (색은 그릴 때 팔레트에서 계산)
    void recolor();
    // 옆 목록과 표시 방식(간결 모드, 거래량 열)을 항상 같게 맞춘다
    void setPeer(StockListWidget *peer) { m_peer = peer; }

signals:
    void detailRequested(const QString &code); // 더블클릭 / [상세 보기]
    void openRequested(const QString &code);   // [네이버 금융에서 보기]
    void removeRequested(const QString &code);
    void manageRequested();

protected:
    void resizeEvent(QResizeEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    struct Fit
    {
        bool full = true;   // 현재가 아래 줄을 "▲ 대비 등락률%"로 다 보여줄 수 있음
        bool volume = true; // 거래량 열까지 보여줄 수 있음
    };
    Fit computeFit() const;
    void updateColumnVisibility();
    // 해당 열의 모든 행을 그리는 데 필요한 폭 (compactSub: 현재가 아래 줄을 간결하게 표시할 때)
    int measureColumn(int col, bool compactSub) const;

    QHash<QString, QTreeWidgetItem *> m_items;
    StockListWidget *m_peer = nullptr;
    bool m_syncing = false;
};
