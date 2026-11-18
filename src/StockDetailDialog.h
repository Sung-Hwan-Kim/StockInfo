#pragma once

#include "ChartData.h"
#include "StockService.h"

#include <QDialog>

class ChartWidget;
class QButtonGroup;
class QCheckBox;
class QLabel;

// 종목 상세 창: 시세 요약 + 차트(캔들, 이동평균, 볼린저, 거래량, MACD, RSI, 스토캐스틱)
// 종목 리스트에서 종목을 더블클릭하면 열린다. 메인 창의 시세 갱신에 맞춰 실시간으로 바뀐다.
class StockDetailDialog : public QDialog
{
    Q_OBJECT
public:
    StockDetailDialog(StockService *service, const QString &code, const QString &name,
                      const QString &market, QWidget *parent = nullptr);

    QString code() const { return m_code; }
    void updateQuote(const Quote &q);

protected:
    void changeEvent(QEvent *e) override;
    void hideEvent(QHideEvent *e) override; // 닫거나 Esc를 눌러도 창 위치 저장

private:
    void loadChart(const QString &timeframe);
    void onChartLoaded(const QString &symbol, const QString &timeframe, const QVector<Candle> &candles, int decimals);
    void onChartFailed(const QString &symbol, const QString &timeframe, const QString &message);
    void applyIndicatorOptions();
    void renderHeader();
    void restoreState();
    void saveState() const;
    QString fmtPrice(double v) const;

    StockService *m_service;
    QString m_code;
    QString m_name;
    QString m_market;

    QLabel *m_title = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_price = nullptr;
    QLabel *m_change = nullptr;
    QLabel *m_stats = nullptr;
    ChartWidget *m_chart = nullptr;

    QButtonGroup *m_tfGroup = nullptr;
    QCheckBox *m_cbMA = nullptr;
    QCheckBox *m_cbBB = nullptr;
    QCheckBox *m_cbVol = nullptr;
    QCheckBox *m_cbMACD = nullptr;
    QCheckBox *m_cbRSI = nullptr;
    QCheckBox *m_cbStoch = nullptr;

    Quote m_quote;
    bool m_hasQuote = false;
    QVector<Candle> m_daily;   // 52주 최고/최저 계산용 일봉
    QString m_timeframe = QStringLiteral("day");
    int m_decimals = 0;
};
