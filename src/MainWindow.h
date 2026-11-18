#pragma once

#include "AppSettings.h"
#include "StockService.h"

#include <QMainWindow>
#include <QPointer>
#include <QTimer>

class IndexPanel;
class SettingsDialog;
class QMenu;
class QSystemTrayIcon;
class StockListWidget;
class QAction;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    // 저장된 위치/크기로 창을 띄운다
    void restoreAndShow();
    // 트레이에 숨어 있거나 최소화된 창을 다시 띄운다 (트레이 클릭, 중복 실행 시)
    void showFromTray();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void closeEvent(QCloseEvent *e) override;
    void moveEvent(QMoveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void changeEvent(QEvent *e) override;

private:
    void buildUi();
    void buildMenus();
    void buildTray();
    bool trayActive() const;
    void hideToTray();
    void toggleVisible();
    void quitApp();
    void applyRuntimeSettings();
    void rebuildLists();
    void refreshNow();
    void onIndexes(const QList<Quote> &quotes);
    void onStocks(const QList<Quote> &quotes);
    void onQuoteFailed(const QString &message, bool isIndex);
    void onCycleFinished();
    void updateStatus();
    void updateWindowTitle();
    void openSettings(bool focusSearch);
    void removeStock(const QString &code);
    void openInBrowser(const QString &code, bool isIndex);
    void setAlwaysOnTop(bool on);
    void setSkin(const QString &skin);
    void showAbout();
    void captureGeometry();
    void scheduleGeometrySave();

    AppSettings m_settings;
    StockService *m_service = nullptr;

    IndexPanel *m_kospiPanel = nullptr;
    IndexPanel *m_kosdaqPanel = nullptr;
    StockListWidget *m_kospiList = nullptr;
    StockListWidget *m_kosdaqList = nullptr;

    QLabel *m_statusLabel = nullptr;
    QLabel *m_versionLabel = nullptr;
    QAction *m_onTopAction = nullptr;
    QAction *m_darkAction = nullptr;
    QAction *m_lightAction = nullptr;
    QPointer<SettingsDialog> m_settingsDialog;

    QAction *m_refreshAction = nullptr;
    QAction *m_prefsAction = nullptr;
    QAction *m_manageAction = nullptr;
    QAction *m_quitAction = nullptr;

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_showHideAction = nullptr;
    bool m_quitting = false; // true면 닫기 버튼도 트레이로 숨기지 않고 종료

    QTimer m_refreshTimer;   // 시세 갱신 주기
    QTimer m_countdownTimer; // 상태 표시줄 카운트다운 (1초)
    QTimer m_geometryTimer;  // 창 위치/크기 저장 지연

    Quote m_kospiQuote;
    Quote m_kosdaqQuote;
    QString m_lastUpdate;
    QStringList m_cycleErrors;
    bool m_restored = false;
};
