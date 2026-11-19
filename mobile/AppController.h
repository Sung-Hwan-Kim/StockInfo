#pragma once

#include "AppSettings.h"
#include "QuoteListModel.h"
#include "StockService.h"

#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

// 모바일 앱의 C++ 쪽 상태. QML에서는 전역 이름 "App"으로 쓴다.
class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *kospiModel READ kospiModel CONSTANT)
    Q_PROPERTY(QObject *kosdaqModel READ kosdaqModel CONSTANT)
    Q_PROPERTY(QObject *service READ service CONSTANT)
    Q_PROPERTY(QVariantMap kospi READ kospi NOTIFY quotesChanged)
    Q_PROPERTY(QVariantMap kosdaq READ kosdaq NOTIFY quotesChanged)
    Q_PROPERTY(int quotesRevision READ quotesRevision NOTIFY quotesChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(QVariantMap colors READ colors NOTIFY darkModeChanged)
    Q_PROPERTY(int refreshSeconds READ refreshSeconds WRITE setRefreshSeconds NOTIFY refreshSecondsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QVariantList stocks READ stocks NOTIFY stocksChanged)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchChanged)
    Q_PROPERTY(QString searchMessage READ searchMessage NOTIFY searchChanged)
    Q_PROPERTY(QString chartTimeframe READ chartTimeframe WRITE setChartTimeframe NOTIFY chartPrefsChanged)
    Q_PROPERTY(QVariantMap chartIndicators READ chartIndicators NOTIFY chartPrefsChanged)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString buildInfo READ buildInfo CONSTANT)
    Q_PROPERTY(QString homepage READ homepage CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);

    QObject *kospiModel() { return &m_kospiModel; }
    QObject *kosdaqModel() { return &m_kosdaqModel; }
    QObject *service() { return m_service; }
    QVariantMap kospi() const { return quote(QStringLiteral("KOSPI")); }
    QVariantMap kosdaq() const { return quote(QStringLiteral("KOSDAQ")); }
    int quotesRevision() const { return m_revision; }

    bool darkMode() const;
    void setDarkMode(bool on);
    QVariantMap colors() const;

    int refreshSeconds() const { return m_settings.refreshSeconds; }
    void setRefreshSeconds(int sec);

    bool busy() const { return m_busy; }
    QString statusText() const;

    QVariantList stocks() const;
    QVariantList searchResults() const { return m_searchResults; }
    bool searching() const { return m_searching; }
    QString searchMessage() const { return m_searchMessage; }

    QString chartTimeframe() const { return m_chartTimeframe; }
    void setChartTimeframe(const QString &tf);
    QVariantMap chartIndicators() const { return m_chartIndicators; }

    QString version() const;
    QString buildInfo() const;
    QString homepage() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE bool addStock(const QString &code, const QString &name, const QString &market);
    Q_INVOKABLE void removeStock(const QString &code);
    Q_INVOKABLE void moveStock(const QString &code, int delta);
    Q_INVOKABLE bool hasStock(const QString &code) const;
    // 시세(표시용 문자열 포함). 아직 없으면 이름/코드만
    Q_INVOKABLE QVariantMap quote(const QString &code) const;
    Q_INVOKABLE void openNaver(const QString &code, bool isIndex);
    Q_INVOKABLE void setChartIndicator(const QString &key, bool on);

signals:
    void quotesChanged();
    void darkModeChanged();
    void refreshSecondsChanged();
    void busyChanged();
    void statusChanged();
    void stocksChanged();
    void searchChanged();
    void chartPrefsChanged();

private:
    void rebuildModels();
    void setBusy(bool b);
    void onIndexes(const QList<Quote> &quotes);
    void onStocks(const QList<Quote> &quotes);
    void loadChartPrefs();
    void saveChartPrefs() const;
    void onAppStateChanged(Qt::ApplicationState state);

    AppSettings m_settings;
    StockService *m_service = nullptr;
    QuoteListModel m_kospiModel;
    QuoteListModel m_kosdaqModel;
    QHash<QString, Quote> m_quotes;
    int m_revision = 0;

    QTimer m_refreshTimer;
    QTimer m_tick; // 상태 표시 (n초 후 갱신)
    bool m_busy = false;
    QString m_lastUpdate;
    QString m_lastError;

    QVariantList m_searchResults;
    bool m_searching = false;
    QString m_searchMessage;
    QString m_lastQuery;

    QString m_chartTimeframe = QStringLiteral("day");
    QVariantMap m_chartIndicators;
};
