#include "MainWindow.h"

#include "IndexPanel.h"
#include "SettingsDialog.h"
#include "StockListWidget.h"
#include "Theme.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QGridLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QScreen>
#include <QStatusBar>
#include <QTime>
#include <QUrl>

namespace {
constexpr int kDefaultWidth = 680;
constexpr int kDefaultHeight = 860;
constexpr int kMinWidth = 360;
constexpr int kMinHeight = 360;

QString marketOfExchange(const QString &exchange)
{
    if (exchange == QLatin1String("KS"))
        return QStringLiteral("KOSPI");
    if (exchange == QLatin1String("KQ"))
        return QStringLiteral("KOSDAQ");
    return {};
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_service(new StockService(this))
{
    m_settings.load();

    buildUi();
    buildMenus();

    connect(m_service, &StockService::indexesUpdated, this, &MainWindow::onIndexes);
    connect(m_service, &StockService::stocksUpdated, this, &MainWindow::onStocks);
    connect(m_service, &StockService::quoteFailed, this, &MainWindow::onQuoteFailed);
    connect(m_service, &StockService::allQuotesFinished, this, &MainWindow::onCycleFinished);

    connect(&m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshNow);
    m_countdownTimer.setInterval(1000);
    connect(&m_countdownTimer, &QTimer::timeout, this, &MainWindow::updateStatus);
    m_geometryTimer.setSingleShot(true);
    m_geometryTimer.setInterval(800);
    connect(&m_geometryTimer, &QTimer::timeout, this, [this] {
        captureGeometry();
        m_settings.save();
    });

    // 프로그램 종료/로그오프 시에도 위치가 저장되도록
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        captureGeometry();
        m_settings.save();
    });

    applyRuntimeSettings();
    updateWindowTitle();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *grid = new QGridLayout(central);
    grid->setContentsMargins(8, 8, 8, 8);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);

    m_kospiPanel = new IndexPanel(QStringLiteral("KOSPI"), tr("코스피"), central);
    m_kosdaqPanel = new IndexPanel(QStringLiteral("KOSDAQ"), tr("코스닥"), central);
    m_kospiList = new StockListWidget(central);
    m_kosdaqList = new StockListWidget(central);

    grid->addWidget(m_kospiPanel, 0, 0);
    grid->addWidget(m_kosdaqPanel, 0, 1);
    grid->addWidget(m_kospiList, 1, 0);
    grid->addWidget(m_kosdaqList, 1, 1);
    // 좌우 같은 폭, 위(지수):아래(종목) = 1:3
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 3);

    setCentralWidget(central);
    setMinimumSize(kMinWidth, kMinHeight);

    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(new QLabel(tr("데이터: 네이버 금융"), this));

    for (IndexPanel *p : {m_kospiPanel, m_kosdaqPanel})
        connect(p, &IndexPanel::doubleClicked, this, [this](const QString &code) { openInBrowser(code, true); });

    for (StockListWidget *list : {m_kospiList, m_kosdaqList}) {
        connect(list, &StockListWidget::openRequested, this, [this](const QString &code) { openInBrowser(code, false); });
        connect(list, &StockListWidget::removeRequested, this, &MainWindow::removeStock);
        connect(list, &StockListWidget::manageRequested, this, [this] { openSettings(true); });
    }
}

void MainWindow::buildMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("파일(&F)"));
    QAction *refresh = fileMenu->addAction(tr("지금 새로고침"), this, &MainWindow::refreshNow);
    refresh->setShortcut(QKeySequence::Refresh);
    fileMenu->addSeparator();
    QAction *quit = fileMenu->addAction(tr("종료"), this, &QWidget::close);
    quit->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));

    QMenu *setMenu = menuBar()->addMenu(tr("설정(&S)"));
    QAction *prefs = setMenu->addAction(tr("환경설정..."), this, [this] { openSettings(false); });
    prefs->setShortcut(QKeySequence(QStringLiteral("Ctrl+,")));
    prefs->setMenuRole(QAction::PreferencesRole);
    QAction *manage = setMenu->addAction(tr("종목 추가/관리..."), this, [this] { openSettings(true); });
    manage->setShortcut(QKeySequence(QStringLiteral("Ctrl+N")));
    setMenu->addSeparator();
    m_onTopAction = setMenu->addAction(tr("항상 위에 표시"));
    m_onTopAction->setCheckable(true);
    m_onTopAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
    connect(m_onTopAction, &QAction::toggled, this, [this](bool on) {
        m_settings.alwaysOnTop = on;
        setAlwaysOnTop(on);
        captureGeometry();
        m_settings.save();
    });

    QMenu *helpMenu = menuBar()->addMenu(tr("도움말(&H)"));
    QAction *about = helpMenu->addAction(tr("정보"), this, [this] {
        QMessageBox::about(this, tr("주식 정보"),
                           tr("<b>코스피 · 코스닥 주식 정보</b><br><br>"
                              "시세 출처: 네이버 금융 (비공식 공개 API)<br>"
                              "표시되는 정보는 참고용이며 실제 시세와 차이가 있을 수 있습니다.<br><br>"
                              "설정 파일: %1<br>Qt %2")
                               .arg(AppSettings::iniPath().toHtmlEscaped(), QString::fromLatin1(qVersion())));
    });
    about->setMenuRole(QAction::AboutRole);
}

void MainWindow::applyRuntimeSettings()
{
    m_kospiList->setListFontSize(m_settings.listFontSize);
    m_kosdaqList->setListFontSize(m_settings.listFontSize);
    rebuildLists();

    {
        const QSignalBlocker block(m_onTopAction);
        m_onTopAction->setChecked(m_settings.alwaysOnTop);
    }
    setAlwaysOnTop(m_settings.alwaysOnTop);

    m_refreshTimer.setInterval(m_settings.refreshSeconds * 1000);
    refreshNow();
    m_countdownTimer.start();
}

void MainWindow::rebuildLists()
{
    QList<StockEntry> kospi, kosdaq;
    for (const StockEntry &e : std::as_const(m_settings.stocks)) {
        if (e.market == QLatin1String("KOSDAQ"))
            kosdaq.append(e);
        else
            kospi.append(e);
    }
    m_kospiList->setStocks(kospi);
    m_kosdaqList->setStocks(kosdaq);
}

void MainWindow::refreshNow()
{
    // 타이머 재시작 (수동 새로고침 후에도 주기 유지)
    m_refreshTimer.start();

    if (m_service->pendingQuoteRequests() > 0)
        return; // 이전 요청이 아직 진행 중

    m_cycleErrors.clear();
    m_service->fetchIndexes();

    QStringList codes;
    for (const StockEntry &e : std::as_const(m_settings.stocks))
        codes << e.code;
    if (!codes.isEmpty())
        m_service->fetchStocks(codes);

    updateStatus();
}

void MainWindow::onIndexes(const QList<Quote> &quotes)
{
    for (const Quote &q : quotes) {
        if (q.code == QLatin1String("KOSPI")) {
            m_kospiQuote = q;
            m_kospiPanel->setQuote(q);
        } else if (q.code == QLatin1String("KOSDAQ")) {
            m_kosdaqQuote = q;
            m_kosdaqPanel->setQuote(q);
        }
    }
    updateWindowTitle();
}

void MainWindow::onStocks(const QList<Quote> &quotes)
{
    // 실제 상장 시장과 등록된 시장이 다르면 올바른 목록으로 옮기고 종목명도 최신으로 갱신
    bool changed = false;
    bool marketChanged = false;
    for (const Quote &q : quotes) {
        for (StockEntry &e : m_settings.stocks) {
            if (e.code != q.code)
                continue;
            const QString market = marketOfExchange(q.exchange);
            if (!market.isEmpty() && market != e.market) {
                e.market = market;
                changed = marketChanged = true;
            }
            if (!q.name.isEmpty() && q.name != e.name) {
                e.name = q.name;
                changed = true;
            }
        }
    }
    if (marketChanged)
        rebuildLists();
    if (changed) {
        captureGeometry();
        m_settings.save();
    }

    for (const Quote &q : quotes) {
        if (m_kospiList->hasCode(q.code))
            m_kospiList->updateQuote(q);
        else if (m_kosdaqList->hasCode(q.code))
            m_kosdaqList->updateQuote(q);
    }
}

void MainWindow::onQuoteFailed(const QString &message, bool isIndex)
{
    m_cycleErrors << message;
    if (isIndex) {
        m_kospiPanel->setStale(true);
        m_kosdaqPanel->setStale(true);
    }
}

void MainWindow::onCycleFinished()
{
    if (m_cycleErrors.isEmpty())
        m_lastUpdate = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
    updateStatus();
}

void MainWindow::updateStatus()
{
    QString text;
    if (!m_lastUpdate.isEmpty())
        text = tr("마지막 갱신 %1").arg(m_lastUpdate);
    else
        text = tr("불러오는 중…");

    if (m_service->pendingQuoteRequests() > 0) {
        text += tr("  ·  갱신 중…");
    } else if (m_refreshTimer.isActive()) {
        const int sec = qMax(0, (m_refreshTimer.remainingTime() + 999) / 1000);
        text += tr("  ·  %1초 후 갱신 (주기 %2초)").arg(sec).arg(m_settings.refreshSeconds);
    }

    if (!m_cycleErrors.isEmpty())
        text += QStringLiteral("  ·  ⚠ ") + m_cycleErrors.last();

    const int count = int(m_settings.stocks.size());
    text += tr("  ·  종목 %1개").arg(count);
    m_statusLabel->setText(text);
}

void MainWindow::updateWindowTitle()
{
    auto part = [](const QString &name, const Quote &q) {
        if (q.price.isEmpty())
            return name;
        return QStringLiteral("%1 %2 %3%")
            .arg(name, q.price, Theme::arrow(q.direction) + QString::number(qAbs(q.rate), 'f', 2));
    };
    setWindowTitle(QStringLiteral("%1 · %2 - %3")
                       .arg(part(tr("코스피"), m_kospiQuote), part(tr("코스닥"), m_kosdaqQuote), tr("주식 정보")));
}

void MainWindow::openSettings(bool focusSearch)
{
    // 이미 열려 있으면 앞으로 가져오기
    if (m_settingsDialog) {
        m_settingsDialog->raise();
        m_settingsDialog->activateWindow();
        if (focusSearch)
            m_settingsDialog->focusSearch();
        return;
    }

    // exec() 대신 open()을 사용: 중첩 이벤트 루프 없이 창 모달로 표시
    auto *dlg = new SettingsDialog(m_settings, m_service, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    m_settingsDialog = dlg;

    auto apply = [this, dlg] {
        dlg->applyTo(m_settings);
        captureGeometry();
        m_settings.save();
        applyRuntimeSettings();
    };
    connect(dlg, &SettingsDialog::applyRequested, this, apply);
    connect(dlg, &QDialog::accepted, this, apply);

    if (focusSearch)
        dlg->focusSearch();
    dlg->open();
}

void MainWindow::removeStock(const QString &code)
{
    for (int i = 0; i < m_settings.stocks.size(); ++i) {
        if (m_settings.stocks[i].code != code)
            continue;
        const QString name = m_settings.stocks[i].name.isEmpty() ? code : m_settings.stocks[i].name;
        if (QMessageBox::question(this, tr("종목 삭제"), tr("'%1' 종목을 목록에서 삭제할까요?").arg(name))
            != QMessageBox::Yes)
            return;
        m_settings.stocks.removeAt(i);
        captureGeometry();
        m_settings.save();
        rebuildLists();
        updateStatus();
        return;
    }
}

void MainWindow::openInBrowser(const QString &code, bool isIndex)
{
    const QString url = isIndex
        ? QStringLiteral("https://finance.naver.com/sise/sise_index.naver?code=%1").arg(code)
        : QStringLiteral("https://finance.naver.com/item/main.naver?code=%1").arg(code);
    QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::setAlwaysOnTop(bool on)
{
    if (bool(windowFlags() & Qt::WindowStaysOnTopHint) == on)
        return;
    const bool visible = isVisible();
    const bool maximized = isMaximized();
    const QRect normal = maximized ? normalGeometry() : geometry();
    // setWindowFlag는 창을 숨기므로 다시 표시하면서 위치를 복원한다
    setWindowFlag(Qt::WindowStaysOnTopHint, on);
    if (visible) {
        setGeometry(normal);
        if (maximized)
            showMaximized();
        else
            show();
    }
}

// ── 창 위치/크기 저장/복원 ──

void MainWindow::restoreAndShow()
{
    const QRect r = m_settings.windowRect;
    bool usable = false;
    if (r.width() >= kMinWidth && r.height() >= kMinHeight) {
        // 제목 표시줄이 어느 모니터에든 충분히 보이는 경우에만 저장된 위치 사용
        const QRect titleBar(r.left(), r.top() - 30, r.width(), 60);
        for (QScreen *s : QGuiApplication::screens()) {
            const QRect inter = s->availableGeometry().intersected(titleBar);
            if (inter.width() >= 100 && inter.height() >= 20) {
                usable = true;
                break;
            }
        }
    }

    if (usable) {
        setGeometry(r);
    } else {
        resize(kDefaultWidth, kDefaultHeight);
        if (QScreen *s = QGuiApplication::primaryScreen()) {
            const QRect avail = s->availableGeometry();
            QSize sz = size().boundedTo(avail.size() - QSize(40, 80));
            resize(sz);
            move(avail.center() - QPoint(sz.width() / 2, sz.height() / 2));
        }
    }

    if (m_settings.windowMaximized)
        showMaximized();
    else
        show();
    m_restored = true;
}

void MainWindow::captureGeometry()
{
    if (!m_restored)
        return;
    m_settings.windowMaximized = isMaximized();
    const QRect g = isMaximized() || isMinimized() ? normalGeometry() : geometry();
    if (g.isValid())
        m_settings.windowRect = g;
}

void MainWindow::scheduleGeometrySave()
{
    if (m_restored)
        m_geometryTimer.start();
}

void MainWindow::moveEvent(QMoveEvent *e)
{
    QMainWindow::moveEvent(e);
    scheduleGeometrySave();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    scheduleGeometrySave();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::WindowStateChange)
        scheduleGeometrySave();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    m_geometryTimer.stop();
    captureGeometry();
    m_settings.save();
    QMainWindow::closeEvent(e);
}
