#pragma once

#include "ChartData.h"

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

// 시세 한 건 (지수 또는 종목)
struct Quote
{
    QString code;         // KOSPI / KOSDAQ / 005930 ...
    QString name;         // 코스피 / 삼성전자 ...
    QString exchange;     // "KS"(코스피) / "KQ"(코스닥) ...
    QString price;        // 현재가 (표시용 문자열, 콤마 포함)
    QString change;       // 전일 대비 (절대값, 표시용)
    double rate = 0.0;    // 등락률 (%, 부호 포함)
    int direction = 0;    // 1 상승, -1 하락, 0 보합
    bool limit = false;   // 상한가/하한가
    QString open;
    QString high;
    QString low;
    QString volume;       // 누적 거래량 (표시용)
    QString tradeValue;   // 누적 거래대금 (표시용)
    QString marketStatus; // OPEN / CLOSE ...
    QDateTime tradedAt;

    // 계산용 수치 (전일 종가 대비 고가/저가 등락률)
    double prevClose = 0.0;   // 전일 종가 = 현재가 - 대비
    double highValue = 0.0;
    double lowValue = 0.0;
    bool hasHighLow = false;  // 장 시작 전 등으로 고가/저가가 없으면 false
    double highRate = 0.0;    // (고가 - 전일종가) / 전일종가 * 100
    double lowRate = 0.0;     // (저가 - 전일종가) / 전일종가 * 100
    int highDirection = 0;    // 고가가 전일 종가보다 높으면 1, 낮으면 -1
    int lowDirection = 0;

    // 상세 창/차트용 수치
    double priceValue = 0.0;
    double openValue = 0.0;
    double volumeValue = 0.0;
    double marketCap = 0.0;   // 시가총액 (원), 종목만
};

// 검색 결과 한 건
struct SearchItem
{
    QString code;
    QString name;
    QString market; // "KOSPI" / "KOSDAQ" / 기타
};

// 네이버 금융 공개 시세 API를 사용하는 데이터 서비스
class StockService : public QObject
{
    Q_OBJECT
public:
    explicit StockService(QObject *parent = nullptr);

    // 코스피/코스닥 지수 요청
    void fetchIndexes();
    // 종목 시세 요청 (여러 종목을 묶어서 요청)
    void fetchStocks(const QStringList &codes);
    // 종목명/코드 검색
    void search(const QString &query);
    // 차트(봉) 데이터 요청. timeframe: "day" / "week" / "month", symbol: 종목코드 또는 KOSPI/KOSDAQ
    void fetchChart(const QString &symbol, const QString &timeframe, int count);

    // 진행 중인 시세 요청 수
    int pendingQuoteRequests() const { return m_pendingQuotes; }

    static QString marketStatusText(const QString &status);

signals:
    void indexesUpdated(const QList<Quote> &quotes);
    void stocksUpdated(const QList<Quote> &quotes);
    void searchFinished(const QString &query, const QList<SearchItem> &items);
    void quoteFailed(const QString &message, bool isIndex);
    void searchFailed(const QString &query, const QString &message);
    // decimals: 가격 소수 자릿수 (종목 0, 지수 2)
    void chartLoaded(const QString &symbol, const QString &timeframe, const QVector<Candle> &candles, int decimals);
    void chartFailed(const QString &symbol, const QString &timeframe, const QString &message);
    void allQuotesFinished();

private:
    QNetworkReply *get(const QString &url);
    void handleQuoteReply(QNetworkReply *reply, bool isIndex);
    static QList<Quote> parseQuotes(const QByteArray &json);

    QNetworkAccessManager *m_nam = nullptr;
    int m_pendingQuotes = 0;
};
