#pragma once

#include "AppSettings.h"
#include "StockService.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;

// 환경설정 창: 갱신 주기, 글꼴 크기, 항상 위, 관심 종목 추가/삭제/순서
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(const AppSettings &settings, StockService *service, QWidget *parent = nullptr);

    // 대화상자의 값을 settings에 반영 (창 위치 등 다른 값은 유지)
    void applyTo(AppSettings &settings) const;

    // 종목 검색창에 포커스
    void focusSearch();

signals:
    void applyRequested();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void startSearch();
    void onSearchFinished(const QString &query, const QList<SearchItem> &items);
    void addSelectedResult();
    void addEntry(const StockEntry &e);
    void removeSelected();
    void moveSelected(int delta);
    void updateButtons();
    static QString entryText(const StockEntry &e);

    StockService *m_service;
    QSpinBox *m_refresh;
    QSpinBox *m_fontSize;
    QCheckBox *m_onTop;
    QComboBox *m_skin;
    QLineEdit *m_searchEdit;
    QPushButton *m_searchBtn;
    QListWidget *m_results;
    QLabel *m_searchStatus;
    QPushButton *m_addBtn;
    QListWidget *m_stocks;
    QPushButton *m_removeBtn;
    QPushButton *m_upBtn;
    QPushButton *m_downBtn;
    QString m_lastQuery;
};
