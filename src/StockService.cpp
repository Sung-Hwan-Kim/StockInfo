#include "StockService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace {

const char kIndexUrl[] = "https://polling.finance.naver.com/api/realtime/domestic/index/";
const char kStockUrl[] = "https://polling.finance.naver.com/api/realtime/domestic/stock/";
const char kSearchUrl[] = "https://ac.stock.naver.com/ac";

constexpr int kCodesPerRequest = 30;
constexpr int kTimeoutMs = 10000;

QString str(const QJsonObject &o, const char *key)
{
    const QJsonValue v = o.value(QLatin1String(key));
    if (v.isString())
        return v.toString();
    if (v.isDouble())
        return QString::number(v.toDouble());
    return {};
}

QString stripSign(QString s)
{
    s = s.trimmed();
    if (s.startsWith(QLatin1Char('-')) || s.startsWith(QLatin1Char('+')))
        s.remove(0, 1);
    return s;
}

} // namespace

StockService::StockService(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

QNetworkReply *StockService::get(const QString &url)
{
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("Mozilla/5.0 (StockInfo Qt viewer)"));
    req.setRawHeader("Accept", "application/json");
    req.setTransferTimeout(kTimeoutMs);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    return m_nam->get(req);
}

void StockService::fetchIndexes()
{
    ++m_pendingQuotes;
    QNetworkReply *reply = get(QString::fromLatin1(kIndexUrl) + QStringLiteral("KOSPI,KOSDAQ"));
    connect(reply, &QNetworkReply::finished, this, [this, reply] { handleQuoteReply(reply, true); });
}

void StockService::fetchStocks(const QStringList &codes)
{
    for (int i = 0; i < codes.size(); i += kCodesPerRequest) {
        const QStringList chunk = codes.mid(i, kCodesPerRequest);
        ++m_pendingQuotes;
        QNetworkReply *reply = get(QString::fromLatin1(kStockUrl) + chunk.join(QLatin1Char(',')));
        connect(reply, &QNetworkReply::finished, this, [this, reply] { handleQuoteReply(reply, false); });
    }
}

void StockService::handleQuoteReply(QNetworkReply *reply, bool isIndex)
{
    reply->deleteLater();
    --m_pendingQuotes;

    if (reply->error() != QNetworkReply::NoError) {
        emit quoteFailed(isIndex ? tr("지수 조회 실패: %1").arg(reply->errorString())
                                 : tr("종목 조회 실패: %1").arg(reply->errorString()),
                         isIndex);
    } else {
        const QList<Quote> quotes = parseQuotes(reply->readAll());
        if (isIndex)
            emit indexesUpdated(quotes);
        else
            emit stocksUpdated(quotes);
    }

    if (m_pendingQuotes == 0)
        emit allQuotesFinished();
}

QList<Quote> StockService::parseQuotes(const QByteArray &json)
{
    QList<Quote> out;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    const QJsonArray datas = doc.object().value(QLatin1String("datas")).toArray();
    for (const QJsonValue &v : datas) {
        const QJsonObject o = v.toObject();
        Quote q;
        q.code = str(o, "itemCode");
        q.name = str(o, "stockName");
        q.exchange = o.value(QLatin1String("stockExchangeType")).toObject().value(QLatin1String("code")).toString();
        q.price = str(o, "closePrice");
        q.change = stripSign(str(o, "compareToPreviousClosePrice"));
        q.open = str(o, "openPrice");
        q.high = str(o, "highPrice");
        q.low = str(o, "lowPrice");
        q.volume = str(o, "accumulatedTradingVolume");
        q.tradeValue = str(o, "accumulatedTradingValue");
        q.marketStatus = str(o, "marketStatus");
        q.tradedAt = QDateTime::fromString(str(o, "localTradedAt"), Qt::ISODateWithMs);
        if (!q.tradedAt.isValid())
            q.tradedAt = QDateTime::fromString(str(o, "localTradedAt"), Qt::ISODate);

        // 등락 구분 코드: 1 상한, 2 상승, 3 보합, 4 하한, 5 하락
        const QString dirCode = o.value(QLatin1String("compareToPreviousPrice")).toObject()
                                    .value(QLatin1String("code")).toString();
        if (dirCode == QLatin1String("1") || dirCode == QLatin1String("2"))
            q.direction = 1;
        else if (dirCode == QLatin1String("4") || dirCode == QLatin1String("5"))
            q.direction = -1;
        else
            q.direction = 0;
        q.limit = (dirCode == QLatin1String("1") || dirCode == QLatin1String("4"));

        QString rateStr = str(o, "fluctuationsRatioRaw");
        if (rateStr.isEmpty())
            rateStr = str(o, "fluctuationsRatio");
        const double absRate = qAbs(stripSign(rateStr).remove(QLatin1Char(',')).toDouble());
        q.rate = q.direction * absRate;

        if (!q.code.isEmpty())
            out.append(q);
    }
    return out;
}

void StockService::search(const QString &query)
{
    const QString q = query.trimmed();
    if (q.isEmpty()) {
        emit searchFinished(query, {});
        return;
    }

    QUrl url(QString::fromLatin1(kSearchUrl));
    QUrlQuery uq;
    uq.addQueryItem(QStringLiteral("q"), q);
    uq.addQueryItem(QStringLiteral("target"), QStringLiteral("stock"));
    url.setQuery(uq);

    QNetworkReply *reply = get(url.toString(QUrl::FullyEncoded));
    connect(reply, &QNetworkReply::finished, this, [this, reply, query] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit searchFailed(query, tr("종목 검색 실패: %1").arg(reply->errorString()));
            emit searchFinished(query, {});
            return;
        }
        QList<SearchItem> items;
        const QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).object()
                                   .value(QLatin1String("items")).toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject o = v.toObject();
            if (o.value(QLatin1String("nationCode")).toString() != QLatin1String("KOR"))
                continue;
            SearchItem it;
            it.code = o.value(QLatin1String("code")).toString();
            it.name = o.value(QLatin1String("name")).toString();
            it.market = o.value(QLatin1String("typeCode")).toString().toUpper();
            if (!it.code.isEmpty())
                items.append(it);
        }
        emit searchFinished(query, items);
    });
}

QString StockService::marketStatusText(const QString &status)
{
    if (status == QLatin1String("OPEN"))
        return tr("장중");
    if (status == QLatin1String("CLOSE"))
        return tr("장마감");
    if (status == QLatin1String("PREOPEN") || status == QLatin1String("PRE_OPEN"))
        return tr("장전");
    if (status.isEmpty())
        return {};
    return status;
}
