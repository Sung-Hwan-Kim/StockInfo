#include "SettingsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr int kCodeRole = Qt::UserRole + 1;
constexpr int kNameRole = Qt::UserRole + 2;
constexpr int kMarketRole = Qt::UserRole + 3;

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
} // namespace

SettingsDialog::SettingsDialog(const AppSettings &settings, StockService *service, QWidget *parent)
    : QDialog(parent)
    , m_service(service)
{
    setWindowTitle(tr("환경설정"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(640, 520);

    // ── 일반 ──
    auto *general = new QGroupBox(tr("일반"), this);
    auto *form = new QFormLayout(general);

    m_refresh = new QSpinBox(general);
    m_refresh->setRange(AppSettings::kMinRefresh, AppSettings::kMaxRefresh);
    m_refresh->setSuffix(tr(" 초"));
    m_refresh->setValue(settings.refreshSeconds);
    m_refresh->setToolTip(tr("시세를 다시 불러오는 간격 (1 ~ 3600초)"));
    form->addRow(tr("갱신 시간:"), m_refresh);

    m_fontSize = new QSpinBox(general);
    m_fontSize->setRange(8, 32);
    m_fontSize->setSuffix(tr(" pt"));
    m_fontSize->setValue(settings.listFontSize);
    form->addRow(tr("종목 목록 글꼴 크기:"), m_fontSize);

    m_onTop = new QCheckBox(tr("항상 위에 표시"), general);
    m_onTop->setChecked(settings.alwaysOnTop);
    form->addRow(QString(), m_onTop);

    // ── 종목 관리 ──
    auto *stockBox = new QGroupBox(tr("보여줄 종목"), this);
    auto *stockLay = new QHBoxLayout(stockBox);

    // 왼쪽: 검색
    auto *searchCol = new QVBoxLayout;
    auto *searchRow = new QHBoxLayout;
    m_searchEdit = new QLineEdit(stockBox);
    m_searchEdit->setPlaceholderText(tr("종목명 또는 코드"));
    m_searchEdit->setToolTip(tr("예: 삼성전자, 005930, 에코프로"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchBtn = new QPushButton(tr("검색"), stockBox);
    searchRow->addWidget(m_searchEdit, 1);
    searchRow->addWidget(m_searchBtn);
    searchCol->addLayout(searchRow);
    m_results = new QListWidget(stockBox);
    m_results->setToolTip(tr("더블클릭하면 추가됩니다"));
    searchCol->addWidget(m_results, 1);
    m_searchStatus = new QLabel(tr("검색어를 입력하고 Enter를 누르세요."), stockBox);
    m_searchStatus->setWordWrap(true);
    searchCol->addWidget(m_searchStatus);

    // 가운데: 추가 버튼
    auto *midCol = new QVBoxLayout;
    midCol->addStretch();
    m_addBtn = new QPushButton(tr("추가 ▶"), stockBox);
    midCol->addWidget(m_addBtn);
    midCol->addStretch();

    // 오른쪽: 등록 종목
    auto *listCol = new QVBoxLayout;
    listCol->addWidget(new QLabel(tr("등록된 종목 (위에서부터 표시)"), stockBox));
    m_stocks = new QListWidget(stockBox);
    listCol->addWidget(m_stocks, 1);
    auto *btnRow = new QHBoxLayout;
    m_upBtn = new QPushButton(tr("▲ 위로"), stockBox);
    m_downBtn = new QPushButton(tr("▼ 아래로"), stockBox);
    m_removeBtn = new QPushButton(tr("삭제"), stockBox);
    btnRow->addWidget(m_upBtn);
    btnRow->addWidget(m_downBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_removeBtn);
    listCol->addLayout(btnRow);

    stockLay->addLayout(searchCol, 1);
    stockLay->addLayout(midCol);
    stockLay->addLayout(listCol, 1);

    for (const StockEntry &e : settings.stocks)
        addEntry(e);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("확인"));
    buttons->button(QDialogButtonBox::Apply)->setText(tr("적용"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("취소"));
    // Enter 키는 검색에 쓰이므로 확인 버튼이 기본 버튼이 되지 않게 한다
    buttons->button(QDialogButtonBox::Ok)->setAutoDefault(false);
    buttons->button(QDialogButtonBox::Ok)->setDefault(false);

    auto *root = new QVBoxLayout(this);
    root->addWidget(general);
    root->addWidget(stockBox, 1);
    root->addWidget(new QLabel(tr("설정 파일: %1").arg(AppSettings::iniPath()), this));
    root->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
            &SettingsDialog::applyRequested);

    connect(m_searchBtn, &QPushButton::clicked, this, &SettingsDialog::startSearch);
    connect(m_service, &StockService::searchFinished, this, &SettingsDialog::onSearchFinished);
    connect(m_service, &StockService::searchFailed, this, [this](const QString &query, const QString &msg) {
        if (query.trimmed() == m_lastQuery)
            m_searchStatus->setText(msg);
    });
    connect(m_results, &QListWidget::itemDoubleClicked, this, &SettingsDialog::addSelectedResult);
    connect(m_addBtn, &QPushButton::clicked, this, &SettingsDialog::addSelectedResult);
    connect(m_removeBtn, &QPushButton::clicked, this, &SettingsDialog::removeSelected);
    connect(m_upBtn, &QPushButton::clicked, this, [this] { moveSelected(-1); });
    connect(m_downBtn, &QPushButton::clicked, this, [this] { moveSelected(1); });
    connect(m_results, &QListWidget::currentRowChanged, this, &SettingsDialog::updateButtons);
    connect(m_stocks, &QListWidget::currentRowChanged, this, &SettingsDialog::updateButtons);

    updateButtons();
}

// Enter 키: 검색창에서는 검색, 검색 결과에서는 추가 (대화상자가 닫히지 않게)
void SettingsDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QWidget *fw = focusWidget();
        if (fw == m_searchEdit) {
            startSearch();
            return;
        }
        if (fw == m_results || fw == m_results->viewport()) {
            addSelectedResult();
            return;
        }
    }
    QDialog::keyPressEvent(e);
}

void SettingsDialog::focusSearch()
{
    // 대화상자가 화면에 나타난 뒤에 포커스를 줘야 첫 입력이 검색창으로 들어간다
    QTimer::singleShot(0, this, [this] {
        m_searchEdit->setFocus(Qt::OtherFocusReason);
        m_searchEdit->selectAll();
    });
}

QString SettingsDialog::entryText(const StockEntry &e)
{
    return QStringLiteral("%1  (%2, %3)").arg(e.name.isEmpty() ? e.code : e.name, e.code, marketLabel(e.market));
}

void SettingsDialog::startSearch()
{
    const QString q = m_searchEdit->text().trimmed();
    if (q.isEmpty())
        return;
    m_lastQuery = q;
    m_results->clear();
    m_searchStatus->setText(tr("'%1' 검색 중…").arg(q));
    m_searchBtn->setEnabled(false);
    m_service->search(q);
}

void SettingsDialog::onSearchFinished(const QString &query, const QList<SearchItem> &items)
{
    if (query.trimmed() != m_lastQuery)
        return; // 이전 검색 결과는 무시
    m_searchBtn->setEnabled(true);
    m_results->clear();

    for (const SearchItem &it : items) {
        auto *li = new QListWidgetItem(
            QStringLiteral("%1  (%2, %3)").arg(it.name, it.code, marketLabel(it.market)), m_results);
        li->setData(kCodeRole, it.code);
        li->setData(kNameRole, it.name);
        li->setData(kMarketRole, it.market);
    }

    if (items.isEmpty()) {
        if (!m_searchStatus->text().contains(tr("실패")))
            m_searchStatus->setText(tr("'%1' 검색 결과가 없습니다.").arg(query));
    } else {
        m_searchStatus->setText(tr("%1건 찾음. 더블클릭 또는 [추가 ▶]").arg(items.size()));
        m_results->setCurrentRow(0);
        m_results->setFocus();
    }
    updateButtons();
}

void SettingsDialog::addSelectedResult()
{
    QListWidgetItem *li = m_results->currentItem();
    if (!li)
        return;
    StockEntry e;
    e.code = li->data(kCodeRole).toString();
    e.name = li->data(kNameRole).toString();
    const QString market = li->data(kMarketRole).toString();
    if (market != QLatin1String("KOSPI") && market != QLatin1String("KOSDAQ")) {
        QMessageBox::information(this, tr("종목 추가"),
                                 tr("%1 은(는) %2 종목입니다.\n코스닥 목록에 표시합니다.")
                                     .arg(e.name, marketLabel(market)));
        e.market = QStringLiteral("KOSDAQ");
    } else {
        e.market = market;
    }
    addEntry(e);
}

void SettingsDialog::addEntry(const StockEntry &e)
{
    for (int i = 0; i < m_stocks->count(); ++i) {
        if (m_stocks->item(i)->data(kCodeRole).toString() == e.code) {
            m_stocks->setCurrentRow(i);
            m_searchStatus->setText(tr("'%1' 은(는) 이미 등록되어 있습니다.").arg(e.name));
            return;
        }
    }
    auto *li = new QListWidgetItem(entryText(e), m_stocks);
    li->setData(kCodeRole, e.code);
    li->setData(kNameRole, e.name);
    li->setData(kMarketRole, e.market);
    m_stocks->setCurrentItem(li);
    updateButtons();
}

void SettingsDialog::removeSelected()
{
    const int row = m_stocks->currentRow();
    if (row < 0)
        return;
    delete m_stocks->takeItem(row);
    if (m_stocks->count() > 0)
        m_stocks->setCurrentRow(qMin(row, m_stocks->count() - 1));
    updateButtons();
}

void SettingsDialog::moveSelected(int delta)
{
    const int row = m_stocks->currentRow();
    const int to = row + delta;
    if (row < 0 || to < 0 || to >= m_stocks->count())
        return;
    QListWidgetItem *li = m_stocks->takeItem(row);
    m_stocks->insertItem(to, li);
    m_stocks->setCurrentRow(to);
}

void SettingsDialog::updateButtons()
{
    const int row = m_stocks->currentRow();
    m_addBtn->setEnabled(m_results->currentItem() != nullptr);
    m_removeBtn->setEnabled(row >= 0);
    m_upBtn->setEnabled(row > 0);
    m_downBtn->setEnabled(row >= 0 && row < m_stocks->count() - 1);
}

void SettingsDialog::applyTo(AppSettings &settings) const
{
    settings.refreshSeconds = m_refresh->value();
    settings.listFontSize = m_fontSize->value();
    settings.alwaysOnTop = m_onTop->isChecked();
    settings.stocks.clear();
    for (int i = 0; i < m_stocks->count(); ++i) {
        const QListWidgetItem *li = m_stocks->item(i);
        settings.stocks.append({li->data(kCodeRole).toString(), li->data(kNameRole).toString(),
                                li->data(kMarketRole).toString()});
    }
}
