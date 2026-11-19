#include "AppController.h"

#include "Skin.h"
#include "Theme.h"

#include <QDate>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QLocale>
#include <QSettings>
#include <QSysInfo>
#include <QTime>
#include <QUrl>

#ifndef STOCKINFO_VERSION
#define STOCKINFO_VERSION "0.0.0"
#endif

namespace {

const QLocale &krLocale()
{
    static const QLocale l(QLocale::Korean, QLocale::SouthKorea);
    return l;
}

QString signedPercent(double v)
{
    return (v > 0 ? QStringLiteral("+") : QString()) + QString::number(v, 'f', 2) + QLatin1Char('%');
}

// 1,131,839,538,508,800 → "1,131조 8,395억"
QString fmtWon(double won)
{
    if (won <= 0)
        return QStringLiteral("-");
    const qint64 eok = qint64(won / 1e8);
    const qint64 jo = eok / 10000;
    const qint64 rest = eok % 10000;
    if (jo > 0)
        return rest > 0 ? QStringLiteral("%1조 %2억").arg(krLocale().toString(jo), krLocale().toString(rest))
                        : QStringLiteral("%1조").arg(krLocale().toString(jo));
    return QStringLiteral("%1억").arg(krLocale().toString(eok));
}

QString marketLabel(const QString &market)
{
    if (market == QLatin1String("KOSPI"))
        return QStringLiteral("코스피");
    if (market == QLatin1String("KOSDAQ"))
        return QStringLiteral("코스닥");
    if (market == QLatin1String("KONEX"))
        return QStringLiteral("코넥스");
    return market;
}

QString marketOfExchange(const QString &exchange)
{
    if (exchange == QLatin1String("KS"))
        return QStringLiteral("KOSPI");
    if (exchange == QLatin1String("KQ"))
        return QStringLiteral("KOSDAQ");
    return {};
}

} // namespace

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_service(new StockService(this))
{
    m_settings.load();
    loadChartPrefs();
    rebuildModels();

    connect(m_service, &StockService::indexesUpdated, this, &AppController::onIndexes);
    connect(m_service, &StockService::stocksUpdated, this, &AppController::onStocks);
    connect(m_service, &StockService::quoteFailed, this, [this](const QString &msg, bool) { m_lastError = msg; });
    connect(m_service, &StockService::allQuotesFinished, this, [this] {
        if (m_lastError.isEmpty())
            m_lastUpdate = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
        setBusy(false);
        emit statusChanged();
    });
    connect(m_service, &StockService::searchFinished, this, [this](const QString &query, const QList<SearchItem> &items) {
        if (query.trimmed() != m_lastQuery)
            return;
        m_searching = false;
        m_searchResults.clear();
        for (const SearchItem &it : items) {
            QVariantMap m;
            m[QStringLiteral("code")] = it.code;
            m[QStringLiteral("name")] = it.name;
            m[QStringLiteral("market")] = it.market;
            m[QStringLiteral("marketLabel")] = marketLabel(it.market);
            m_searchResults.append(m);
        }
        if (items.isEmpty() && m_searchMessage.isEmpty())
            m_searchMessage = QStringLiteral("'%1' 검색 결과가 없습니다.").arg(query);
        else if (!items.isEmpty())
            m_searchMessage = QStringLiteral("%1건 찾음 · 눌러서 추가").arg(items.size());
        emit searchChanged();
    });
    connect(m_service, &StockService::searchFailed, this, [this](const QString &query, const QString &msg) {
        if (query.trimmed() == m_lastQuery)
            m_searchMessage = msg;
    });

    m_refreshTimer.setInterval(m_settings.refreshSeconds * 1000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &AppController::refresh);
    m_tick.setInterval(1000);
    connect(&m_tick, &QTimer::timeout, this, &AppController::statusChanged);
    m_tick.start();

    // 앱이 백그라운드로 가면 갱신을 멈추고, 돌아오면 바로 갱신 (배터리 절약)
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &AppController::onAppStateChanged);

    refresh();
}

// ── 시세 ──

void AppController::refresh()
{
    m_refreshTimer.start(); // 주기 다시 시작
    if (m_service->pendingQuoteRequests() > 0)
        return;
    m_lastError.clear();
    setBusy(true);
    m_service->fetchIndexes();
    QStringList codes;
    for (const StockEntry &e : std::as_const(m_settings.stocks))
        codes << e.code;
    if (!codes.isEmpty())
        m_service->fetchStocks(codes);
    emit statusChanged();
}

void AppController::onIndexes(const QList<Quote> &quotes)
{
    for (const Quote &q : quotes)
        m_quotes.insert(q.code, q);
    ++m_revision;
    emit quotesChanged();
}

void AppController::onStocks(const QList<Quote> &quotes)
{
    // 실제 시장/종목명이 다르면 바로잡기
    bool marketChanged = false, changed = false;
    for (const Quote &q : quotes) {
        for (StockEntry &e : m_settings.stocks) {
            if (e.code != q.code)
                continue;
            const QString market = marketOfExchange(q.exchange);
            if (!market.isEmpty() && market != e.market) {
                e.market = market;
                marketChanged = changed = true;
            }
            if (!q.name.isEmpty() && q.name != e.name) {
                e.name = q.name;
                changed = true;
            }
        }
    }
    if (marketChanged)
        rebuildModels();
    if (changed) {
        m_settings.save();
        emit stocksChanged();
    }

    for (const Quote &q : quotes) {
        m_quotes.insert(q.code, q);
        if (!m_kospiModel.updateQuote(q))
            m_kosdaqModel.updateQuote(q);
    }
    ++m_revision;
    emit quotesChanged();
}

QVariantMap AppController::quote(const QString &code) const
{
    QVariantMap m;
    m[QStringLiteral("code")] = code;
    const bool isIndex = (code == QLatin1String("KOSPI") || code == QLatin1String("KOSDAQ"));
    QString name = code == QLatin1String("KOSPI") ? QStringLiteral("코스피")
                 : code == QLatin1String("KOSDAQ") ? QStringLiteral("코스닥") : code;
    QString market = isIndex ? code : QStringLiteral("KOSPI");
    for (const StockEntry &e : m_settings.stocks) {
        if (e.code == code) {
            name = e.name.isEmpty() ? code : e.name;
            market = e.market;
        }
    }
    m[QStringLiteral("name")] = name;
    m[QStringLiteral("market")] = market;
    m[QStringLiteral("marketLabel")] = isIndex ? QStringLiteral("지수") : marketLabel(market);
    m[QStringLiteral("isIndex")] = isIndex;

    const auto it = m_quotes.constFind(code);
    m[QStringLiteral("hasQuote")] = it != m_quotes.constEnd();
    if (it == m_quotes.constEnd())
        return m;

    const Quote &q = *it;
    const int decimals = isIndex ? 2 : 0;
    if (!isIndex && !q.name.isEmpty())
        m[QStringLiteral("name")] = q.name;
    m[QStringLiteral("price")] = q.price;
    m[QStringLiteral("changeText")] = Theme::arrowText(q.direction, q.change);
    m[QStringLiteral("rateText")] = signedPercent(q.rate);
    m[QStringLiteral("direction")] = q.direction;
    m[QStringLiteral("open")] = (q.open.isEmpty() || q.open == QLatin1String("-")) ? QStringLiteral("-") : q.open;
    m[QStringLiteral("hasHighLow")] = q.hasHighLow;
    m[QStringLiteral("high")] = q.hasHighLow ? q.high : QStringLiteral("-");
    m[QStringLiteral("low")] = q.hasHighLow ? q.low : QStringLiteral("-");
    m[QStringLiteral("highRateText")] = signedPercent(q.highRate);
    m[QStringLiteral("lowRateText")] = signedPercent(q.lowRate);
    m[QStringLiteral("highDir")] = q.highDirection;
    m[QStringLiteral("lowDir")] = q.lowDirection;
    m[QStringLiteral("prevClose")] = q.prevClose > 0 ? krLocale().toString(q.prevClose, 'f', decimals) : QStringLiteral("-");
    m[QStringLiteral("volume")] = (q.volume.isEmpty() || q.volume == QLatin1String("-")) ? QStringLiteral("-") : q.volume;
    m[QStringLiteral("tradeValue")] = (q.tradeValue.isEmpty() || q.tradeValue == QLatin1String("-")) ? QStringLiteral("-") : q.tradeValue;
    m[QStringLiteral("marketCap")] = fmtWon(q.marketCap);
    QString status = StockService::marketStatusText(q.marketStatus);
    if (q.tradedAt.isValid())
        status += QStringLiteral(" · ") + q.tradedAt.toString(QStringLiteral("MM/dd HH:mm:ss"));
    m[QStringLiteral("status")] = status;
    return m;
}

void AppController::setBusy(bool b)
{
    if (m_busy != b) {
        m_busy = b;
        emit busyChanged();
    }
}

QString AppController::statusText() const
{
    QString t = m_lastUpdate.isEmpty() ? QStringLiteral("불러오는 중…")
                                       : QStringLiteral("마지막 갱신 %1").arg(m_lastUpdate);
    if (m_busy) {
        t += QStringLiteral(" · 갱신 중…");
    } else if (m_refreshTimer.isActive()) {
        const int sec = qMax(0, (m_refreshTimer.remainingTime() + 999) / 1000);
        t += QStringLiteral(" · %1초 후 갱신").arg(sec);
    } else {
        t += QStringLiteral(" · 일시 정지");
    }
    if (!m_lastError.isEmpty())
        t += QStringLiteral(" · ⚠ ") + m_lastError;
    return t;
}

void AppController::onAppStateChanged(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationActive) {
        refresh();
    } else if (state == Qt::ApplicationSuspended || state == Qt::ApplicationHidden) {
        m_refreshTimer.stop();
        emit statusChanged();
    }
}

// ── 설정 ──

bool AppController::darkMode() const
{
    return Skin::fromString(m_settings.skin) == Skin::Mode::Dark;
}

void AppController::setDarkMode(bool on)
{
    if (darkMode() == on)
        return;
    m_settings.skin = Skin::toString(on ? Skin::Mode::Dark : Skin::Mode::Light);
    m_settings.save();
    emit darkModeChanged();
}

QVariantMap AppController::colors() const
{
    const QPalette pal = Skin::palette(darkMode() ? Skin::Mode::Dark : Skin::Mode::Light);
    const bool dark = darkMode();
    QVariantMap m;
    m[QStringLiteral("window")] = pal.color(QPalette::Window);
    m[QStringLiteral("base")] = pal.color(QPalette::Base);
    m[QStringLiteral("alt")] = pal.color(QPalette::AlternateBase);
    m[QStringLiteral("text")] = pal.color(QPalette::Text);
    QColor dim = pal.color(QPalette::Text);
    dim.setAlphaF(0.6f);
    m[QStringLiteral("dim")] = dim;
    m[QStringLiteral("border")] = dark ? QColor(0x3a, 0x3d, 0x42) : QColor(0xc8, 0xcb, 0xd0);
    m[QStringLiteral("header")] = dark ? QColor(0x2d, 0x30, 0x35) : QColor(0xea, 0xec, 0xef);
    m[QStringLiteral("pressed")] = dark ? QColor(0x37, 0x3b, 0x42) : QColor(0xdd, 0xe3, 0xec);
    m[QStringLiteral("up")] = Theme::directionColor(pal, 1);
    m[QStringLiteral("down")] = Theme::directionColor(pal, -1);
    m[QStringLiteral("accent")] = pal.color(QPalette::Highlight);
    m[QStringLiteral("link")] = pal.color(QPalette::Link);
    return m;
}

void AppController::setRefreshSeconds(int sec)
{
    sec = qBound(AppSettings::kMinRefresh, sec, AppSettings::kMaxRefresh);
    if (sec == m_settings.refreshSeconds)
        return;
    m_settings.refreshSeconds = sec;
    m_settings.save();
    m_refreshTimer.setInterval(sec * 1000);
    if (m_refreshTimer.isActive())
        m_refreshTimer.start();
    emit refreshSecondsChanged();
    emit statusChanged();
}

// ── 종목 관리 ──

void AppController::rebuildModels()
{
    QList<StockEntry> kospi, kosdaq;
    for (const StockEntry &e : std::as_const(m_settings.stocks))
        (e.market == QLatin1String("KOSDAQ") ? kosdaq : kospi).append(e);
    m_kospiModel.setEntries(kospi);
    m_kosdaqModel.setEntries(kosdaq);
    for (const Quote &q : std::as_const(m_quotes)) {
        if (!m_kospiModel.updateQuote(q))
            m_kosdaqModel.updateQuote(q);
    }
}

QVariantList AppController::stocks() const
{
    QVariantList list;
    for (const StockEntry &e : m_settings.stocks) {
        QVariantMap m;
        m[QStringLiteral("code")] = e.code;
        m[QStringLiteral("name")] = e.name.isEmpty() ? e.code : e.name;
        m[QStringLiteral("market")] = e.market;
        m[QStringLiteral("marketLabel")] = marketLabel(e.market);
        list.append(m);
    }
    return list;
}

bool AppController::hasStock(const QString &code) const
{
    for (const StockEntry &e : m_settings.stocks)
        if (e.code == code)
            return true;
    return false;
}

void AppController::search(const QString &query)
{
    const QString q = query.trimmed();
    if (q.isEmpty())
        return;
    m_lastQuery = q;
    m_searching = true;
    m_searchMessage.clear();
    m_searchResults.clear();
    emit searchChanged();
    m_service->search(q);
}

bool AppController::addStock(const QString &code, const QString &name, const QString &market)
{
    if (code.isEmpty() || hasStock(code))
        return false;
    StockEntry e;
    e.code = code;
    e.name = name;
    // 코스피/코스닥이 아니면 (코넥스 등) 코스닥 목록에 표시
    e.market = (market == QLatin1String("KOSPI")) ? market : QStringLiteral("KOSDAQ");
    m_settings.stocks.append(e);
    m_settings.save();
    rebuildModels();
    emit stocksChanged();
    refresh();
    return true;
}

void AppController::removeStock(const QString &code)
{
    for (int i = 0; i < m_settings.stocks.size(); ++i) {
        if (m_settings.stocks[i].code == code) {
            m_settings.stocks.removeAt(i);
            m_settings.save();
            rebuildModels();
            emit stocksChanged();
            return;
        }
    }
}

void AppController::moveStock(const QString &code, int delta)
{
    for (int i = 0; i < m_settings.stocks.size(); ++i) {
        if (m_settings.stocks[i].code != code)
            continue;
        const int to = i + delta;
        if (to < 0 || to >= m_settings.stocks.size())
            return;
        m_settings.stocks.move(i, to);
        m_settings.save();
        rebuildModels();
        emit stocksChanged();
        return;
    }
}

void AppController::openNaver(const QString &code, bool isIndex)
{
    const QString url = isIndex
        ? QStringLiteral("https://finance.naver.com/sise/sise_index.naver?code=%1").arg(code)
        : QStringLiteral("https://finance.naver.com/item/main.naver?code=%1").arg(code);
    QDesktopServices::openUrl(QUrl(url));
}

// ── 차트 설정 (PC와 같은 INI [DetailWindow] 그룹) ──

void AppController::loadChartPrefs()
{
    QSettings s(AppSettings::iniPath(), QSettings::IniFormat);
    s.beginGroup(QStringLiteral("DetailWindow"));
    m_chartTimeframe = s.value(QStringLiteral("Timeframe"), QStringLiteral("day")).toString();
    if (m_chartTimeframe != QLatin1String("week") && m_chartTimeframe != QLatin1String("month"))
        m_chartTimeframe = QStringLiteral("day");
    const QList<QPair<QString, bool>> keys = {{QStringLiteral("MA"), true},     {QStringLiteral("Bollinger"), false},
                                             {QStringLiteral("Volume"), true}, {QStringLiteral("MACD"), true},
                                             {QStringLiteral("RSI"), true},    {QStringLiteral("Stochastic"), false}};
    for (const auto &k : keys)
        m_chartIndicators[k.first] = s.value(k.first, k.second).toBool();
    s.endGroup();
}

void AppController::saveChartPrefs() const
{
    QSettings s(AppSettings::iniPath(), QSettings::IniFormat);
    s.beginGroup(QStringLiteral("DetailWindow"));
    s.setValue(QStringLiteral("Timeframe"), m_chartTimeframe);
    for (auto it = m_chartIndicators.constBegin(); it != m_chartIndicators.constEnd(); ++it)
        s.setValue(it.key(), it.value().toBool());
    s.endGroup();
}

void AppController::setChartTimeframe(const QString &tf)
{
    if (tf == m_chartTimeframe)
        return;
    m_chartTimeframe = tf;
    saveChartPrefs();
    emit chartPrefsChanged();
}

void AppController::setChartIndicator(const QString &key, bool on)
{
    if (m_chartIndicators.value(key).toBool() == on)
        return;
    m_chartIndicators[key] = on;
    saveChartPrefs();
    emit chartPrefsChanged();
}

// ── 정보 ──

QString AppController::version() const
{
    return QStringLiteral(STOCKINFO_VERSION);
}

QString AppController::buildInfo() const
{
    const QString raw = QString::fromLatin1(__DATE__).simplified();
    const QDate d = QLocale::c().toDate(raw, QStringLiteral("MMM d yyyy"));
    return QStringLiteral("빌드 %1 · Qt %2\n%3 (%4)")
        .arg(d.isValid() ? d.toString(Qt::ISODate) : raw, QString::fromLatin1(qVersion()),
             QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
}

QString AppController::homepage() const
{
    return QStringLiteral("https://github.com/Sung-Hwan-Kim/StockInfo");
}
