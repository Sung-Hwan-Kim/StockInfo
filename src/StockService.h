#pragma once

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

    // 진행 중인 시세 요청 수
    int pendingQuoteRequests() const { return m_pendingQuotes; }

    static QString marketStatusText(const QString &status);

signals:
    void indexesUpdated(const QList<Quote> &quotes);
    void stocksUpdated(const QList<Quote> &quotes);
    void searchFinished(const QString &query, const QList<SearchItem> &items);
    void quoteFailed(const QString &message, bool isIndex);
    void searchFailed(const QString &query, const QString &message);
    void allQuotesFinished();

private:
    QNetworkReply *get(const QString &url);
    void handleQuoteReply(QNetworkReply *reply, bool isIndex);
    static QList<Quote> parseQuotes(const QByteArray &json);

    QNetworkAccessManager *m_nam = nullptr;
    int m_pendingQuotes = 0;
};
