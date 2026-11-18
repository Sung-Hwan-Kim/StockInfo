#include "StockDetailDialog.h"

#include "AppSettings.h"
#include "ChartWidget.h"
#include "Theme.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QHideEvent>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

namespace {

const QLocale &krLocale()
{
    static const QLocale l(QLocale::Korean, QLocale::SouthKorea);
    return l;
}

// 1,131,839,538,508,800 → "1,131조 8,395억"
QString fmtWon(double won)
{
    if (won <= 0)
        return QStringLiteral("-");
    const qint64 eok = qint64(won / 1e8); // 억 단위
    const qint64 jo = eok / 10000;
    const qint64 rest = eok % 10000;
    if (jo > 0)
        return rest > 0 ? QStringLiteral("%1조 %2억").arg(krLocale().toString(jo), krLocale().toString(rest))
                        : QStringLiteral("%1조").arg(krLocale().toString(jo));
    return QStringLiteral("%1억").arg(krLocale().toString(eok));
}

QString signedPercent(double v)
{
    return (v > 0 ? QStringLiteral("+") : QString()) + QString::number(v, 'f', 2) + QLatin1Char('%');
}

QString colored(const QString &text, const QColor &c, bool bold = false)
{
    return QStringLiteral("<span style=\"color:%1;%2\">%3</span>")
        .arg(c.name(), bold ? QStringLiteral("font-weight:600;") : QString(), text.toHtmlEscaped());
}

// HTML에서는 반투명 색을 쓸 수 없으므로 글자색과 배경색을 섞은 흐린 색
QColor solidDim(const QPalette &pal)
{
    const QColor a = pal.color(QPalette::WindowText);
    const QColor b = pal.color(QPalette::Window);
    const double t = 0.62;
    return QColor::fromRgbF(float(a.redF() * t + b.redF() * (1 - t)), float(a.greenF() * t + b.greenF() * (1 - t)),
                            float(a.blueF() * t + b.blueF() * (1 - t)));
}

int chartCount(const QString &tf)
{
    if (tf == QLatin1String("week"))
        return 300;
    if (tf == QLatin1String("month"))
        return 240;
    return 500;
}

// 봉 주기 선택 버튼: 선택된 버튼을 강조색으로 채워서 다크 모드에서도 잘 보이게
class SegmentButton : public QPushButton
{
public:
    using QPushButton::QPushButton;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QPalette pal = palette();
        const bool dark = Theme::isDark(pal);
        const bool on = isChecked();
        const QRectF r = QRectF(rect()).adjusted(0.5, 1.5, -0.5, -1.5);

        QColor bg = on ? pal.color(QPalette::Highlight) : pal.color(QPalette::Button);
        if (!on && underMouse())
            bg = dark ? bg.lighter(130) : bg.darker(106);
        if (isDown())
            bg = dark ? bg.lighter(150) : bg.darker(112);
        const QColor border = on ? pal.color(QPalette::Highlight)
                                 : (dark ? pal.color(QPalette::Light) : pal.color(QPalette::Mid));
        p.setPen(QPen(border, 1));
        p.setBrush(bg);
        p.drawRoundedRect(r, 4, 4);

        QFont f = font();
        f.setBold(on);
        p.setFont(f);
        p.setPen(on ? pal.color(QPalette::HighlightedText) : pal.color(QPalette::ButtonText));
        p.drawText(r, Qt::AlignCenter, text());
    }
};

ChartWidget::Timeframe toTimeframe(const QString &tf)
{
    if (tf == QLatin1String("week"))
        return ChartWidget::Timeframe::Week;
    if (tf == QLatin1String("month"))
        return ChartWidget::Timeframe::Month;
    return ChartWidget::Timeframe::Day;
}

} // namespace

StockDetailDialog::StockDetailDialog(StockService *service, const QString &code, const QString &name,
                                     const QString &market, QWidget *parent)
    : QDialog(parent)
    , m_service(service)
    , m_code(code)
    , m_name(name.isEmpty() ? code : name)
    , m_market(market)
{
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("%1 (%2) - 종목 상세").arg(m_name, m_code));

    // ── 머리: 종목명 / 현재가 ──
    m_title = new QLabel(this);
    m_title->setTextFormat(Qt::RichText);
    m_status = new QLabel(this);
    m_status->setTextFormat(Qt::RichText);
    m_price = new QLabel(this);
    m_price->setTextFormat(Qt::RichText);
    m_price->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_change = new QLabel(this);
    m_change->setTextFormat(Qt::RichText);
    m_change->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *nameCol = new QVBoxLayout;
    nameCol->setSpacing(2);
    nameCol->addWidget(m_title);
    nameCol->addWidget(m_status);
    auto *priceCol = new QVBoxLayout;
    priceCol->setSpacing(0);
    priceCol->addWidget(m_price);
    priceCol->addWidget(m_change);
    auto *head = new QHBoxLayout;
    head->addLayout(nameCol, 1);
    head->addLayout(priceCol);

    // ── 시세 요약 ──
    m_stats = new QLabel(this);
    m_stats->setTextFormat(Qt::RichText);
    m_stats->setTextInteractionFlags(Qt::TextSelectableByMouse);

    // ── 차트 옵션 ──
    auto *options = new QHBoxLayout;
    options->setSpacing(6);
    m_tfGroup = new QButtonGroup(this);
    m_tfGroup->setExclusive(true);
    const QList<QPair<QString, QString>> tfs = {{QStringLiteral("day"), tr("일봉")},
                                               {QStringLiteral("week"), tr("주봉")},
                                               {QStringLiteral("month"), tr("월봉")}};
    for (const auto &tf : tfs) {
        auto *b = new SegmentButton(tf.second, this);
        b->setCheckable(true);
        b->setAutoDefault(false);            // 대화상자 기본 버튼 강조 없음
        b->setFocusPolicy(Qt::TabFocus);
        b->setAttribute(Qt::WA_Hover);
        b->setMinimumHeight(26);
        b->setProperty("timeframe", tf.first);
        b->setMinimumWidth(56);
        m_tfGroup->addButton(b);
        options->addWidget(b);
    }
    options->addSpacing(14);
    m_cbMA = new QCheckBox(tr("이동평균"), this);
    m_cbMA->setToolTip(tr("이동평균선 5 · 20 · 60 · 120"));
    m_cbBB = new QCheckBox(tr("볼린저밴드"), this);
    m_cbBB->setToolTip(tr("볼린저 밴드 (20, 2)"));
    m_cbVol = new QCheckBox(tr("거래량"), this);
    m_cbMACD = new QCheckBox(QStringLiteral("MACD"), this);
    m_cbMACD->setToolTip(tr("MACD (12, 26, 9): MACD선, 시그널선, 오실레이터(OSC) 막대"));
    m_cbRSI = new QCheckBox(QStringLiteral("RSI"), this);
    m_cbRSI->setToolTip(tr("RSI (14)"));
    m_cbStoch = new QCheckBox(tr("스토캐스틱"), this);
    m_cbStoch->setToolTip(tr("스토캐스틱 슬로우 (12, 5, 5)"));
    for (QCheckBox *cb : {m_cbMA, m_cbBB, m_cbVol, m_cbMACD, m_cbRSI, m_cbStoch}) {
        options->addWidget(cb);
        connect(cb, &QCheckBox::toggled, this, &StockDetailDialog::applyIndicatorOptions);
    }
    options->addStretch();
    auto *naver = new QPushButton(tr("네이버 금융"), this);
    naver->setAutoDefault(false);
    naver->setToolTip(tr("네이버 금융에서 이 종목 보기"));
    connect(naver, &QPushButton::clicked, this, [this] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://finance.naver.com/item/main.naver?code=%1").arg(m_code)));
    });
    options->addWidget(naver);

    // ── 차트 ──
    m_chart = new ChartWidget(this);
    m_chart->setMessage(tr("차트 불러오는 중…"));

    auto *hint = new QLabel(tr("마우스 휠: 확대/축소 · 끌기: 이동 · 더블클릭: 처음 보기"), this);
    QPalette hp = hint->palette();
    hp.setColor(QPalette::WindowText, Theme::dimText(palette()));
    hint->setPalette(hp);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 12, 14, 10);
    root->setSpacing(8);
    root->addLayout(head);
    root->addWidget(m_stats);
    root->addLayout(options);
    root->addWidget(m_chart, 1);
    root->addWidget(hint);

    connect(m_service, &StockService::chartLoaded, this, &StockDetailDialog::onChartLoaded);
    connect(m_service, &StockService::chartFailed, this, &StockDetailDialog::onChartFailed);
    connect(m_tfGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton *b) {
        loadChart(b->property("timeframe").toString());
        saveState();
    });

    restoreState();
    renderHeader();

    // 선택한 봉 주기 차트 + (주봉/월봉이면) 52주 최고/최저용 일봉
    loadChart(m_timeframe);
    if (m_timeframe != QLatin1String("day"))
        m_service->fetchChart(m_code, QStringLiteral("day"), 260);
}

// ── 시세 ──

void StockDetailDialog::updateQuote(const Quote &q)
{
    m_quote = q;
    m_hasQuote = true;
    if (!q.name.isEmpty() && q.name != m_name) {
        m_name = q.name;
        setWindowTitle(tr("%1 (%2) - 종목 상세").arg(m_name, m_code));
    }
    renderHeader();

    // 오늘 봉을 실시간 시세로 갱신
    if (q.hasHighLow && q.priceValue > 0) {
        const QDate today = q.tradedAt.isValid() ? q.tradedAt.date() : QDate::currentDate();
        m_chart->applyLive(today, q.openValue, q.highValue, q.lowValue, q.priceValue, q.volumeValue);
    }
}

QString StockDetailDialog::fmtPrice(double v) const
{
    return krLocale().toString(v, 'f', m_decimals);
}

void StockDetailDialog::renderHeader()
{
    const QPalette pal = palette();
    const QColor text = pal.color(QPalette::WindowText);
    const QColor dim = solidDim(pal);
    const QString marketName = m_market == QLatin1String("KOSDAQ") ? tr("코스닥") : tr("코스피");

    QFont tf = font();
    m_title->setText(QStringLiteral("<span style=\"font-size:%1pt; font-weight:700;\">%2</span>"
                                    "&nbsp;&nbsp;<span style=\"color:%3;\">%4 · %5</span>")
                         .arg(QString::number(tf.pointSizeF() * 1.7, 'f', 1), m_name.toHtmlEscaped(),
                              dim.name(), m_code, marketName));

    if (!m_hasQuote) {
        m_status->setText(colored(tr("시세 불러오는 중…"), dim));
        m_price->clear();
        m_change->clear();
        m_stats->clear();
        return;
    }

    const Quote &q = m_quote;
    QString status = StockService::marketStatusText(q.marketStatus);
    if (q.tradedAt.isValid())
        status += QStringLiteral(" · ") + q.tradedAt.toString(QStringLiteral("MM/dd HH:mm:ss"));
    m_status->setText(colored(status, dim));

    const QColor dirColor = q.direction == 0 ? text : Theme::directionColor(pal, q.direction);
    m_price->setText(QStringLiteral("<span style=\"font-size:%1pt; font-weight:700; color:%2;\">%3</span>")
                         .arg(QString::number(tf.pointSizeF() * 2.4, 'f', 1), dirColor.name(), q.price.toHtmlEscaped()));
    m_change->setText(colored(Theme::arrowText(q.direction, q.change) + QStringLiteral("   ") + signedPercent(q.rate),
                              dirColor, true));

    // 52주 최고/최저 (일봉 + 오늘 시세)
    double hi52 = 0, lo52 = 0;
    QDate hiDate, loDate;
    const QDate from = QDate::currentDate().addDays(-365);
    for (const Candle &c : std::as_const(m_daily)) {
        if (c.date < from)
            continue;
        if (hi52 == 0 || c.high > hi52) { hi52 = c.high; hiDate = c.date; }
        if (lo52 == 0 || c.low < lo52) { lo52 = c.low; loDate = c.date; }
    }
    if (q.hasHighLow) {
        const QDate today = q.tradedAt.isValid() ? q.tradedAt.date() : QDate::currentDate();
        if (hi52 == 0 || q.highValue > hi52) { hi52 = q.highValue; hiDate = today; }
        if (lo52 == 0 || q.lowValue < lo52) { lo52 = q.lowValue; loDate = today; }
    }

    auto cell = [&](const QString &label, const QString &value) {
        return QStringLiteral("<td style=\"color:%1; padding-right:6px;\">%2</td><td style=\"padding-right:22px;\">%3</td>")
            .arg(dim.name(), label.toHtmlEscaped(), value);
    };
    auto withRate = [&](const QString &value, double rate, int dir) {
        const QColor c = dir == 0 ? text : Theme::directionColor(pal, dir);
        return colored(value, c, true) + QStringLiteral(" ") + colored(QStringLiteral("(") + signedPercent(rate) + QStringLiteral(")"), c);
    };

    const QString open = q.open.isEmpty() || q.open == QLatin1String("-") ? QStringLiteral("-") : colored(q.open, text, true);
    const QString high = q.hasHighLow ? withRate(q.high, q.highRate, q.highDirection) : QStringLiteral("-");
    const QString low = q.hasHighLow ? withRate(q.low, q.lowRate, q.lowDirection) : QStringLiteral("-");
    const QString prev = q.prevClose > 0 ? colored(fmtPrice(q.prevClose), text, true) : QStringLiteral("-");
    const QString volume = q.volume.isEmpty() ? QStringLiteral("-") : colored(q.volume, text, true);
    const QString value = q.tradeValue.isEmpty() ? QStringLiteral("-") : colored(q.tradeValue, text, true);
    const QString cap = colored(fmtWon(q.marketCap), text, true);
    const QString w52 = hi52 > 0
        ? colored(fmtPrice(hi52), Theme::directionColor(pal, 1), true) + colored(QStringLiteral(" (") + hiDate.toString(QStringLiteral("yy.MM.dd")) + QStringLiteral(")"), dim)
              + QStringLiteral(" / ")
              + colored(fmtPrice(lo52), Theme::directionColor(pal, -1), true) + colored(QStringLiteral(" (") + loDate.toString(QStringLiteral("yy.MM.dd")) + QStringLiteral(")"), dim)
        : colored(tr("불러오는 중…"), dim);

    m_stats->setText(QStringLiteral("<table cellspacing=\"0\" cellpadding=\"2\">"
                                    "<tr>%1%2%3%4</tr><tr>%5%6%7%8</tr></table>")
                         .arg(cell(tr("시가"), open), cell(tr("고가"), high), cell(tr("저가"), low), cell(tr("전일 종가"), prev),
                              cell(tr("거래량"), volume), cell(tr("거래대금"), value), cell(tr("시가총액"), cap),
                              cell(tr("52주 최고/최저"), w52)));
}

// ── 차트 ──

void StockDetailDialog::loadChart(const QString &timeframe)
{
    m_timeframe = timeframe;
    for (QAbstractButton *b : m_tfGroup->buttons())
        b->setChecked(b->property("timeframe").toString() == timeframe);
    m_chart->setMessage(tr("차트 불러오는 중…"));
    m_service->fetchChart(m_code, timeframe, chartCount(timeframe));
}

void StockDetailDialog::onChartLoaded(const QString &symbol, const QString &timeframe,
                                      const QVector<Candle> &candles, int decimals)
{
    if (symbol != m_code)
        return;
    if (timeframe == QLatin1String("day")) {
        m_daily = candles;
        m_decimals = decimals;
        renderHeader();
    }
    if (timeframe != m_timeframe)
        return;

    m_decimals = decimals;
    m_chart->setCandles(candles, toTimeframe(timeframe), decimals);
    // 받은 차트가 오늘 시세보다 늦으면 실시간 값으로 맞춘다
    if (m_hasQuote && m_quote.hasHighLow && m_quote.priceValue > 0) {
        const QDate today = m_quote.tradedAt.isValid() ? m_quote.tradedAt.date() : QDate::currentDate();
        m_chart->applyLive(today, m_quote.openValue, m_quote.highValue, m_quote.lowValue,
                           m_quote.priceValue, m_quote.volumeValue);
    }
}

void StockDetailDialog::onChartFailed(const QString &symbol, const QString &timeframe, const QString &message)
{
    if (symbol == m_code && timeframe == m_timeframe)
        m_chart->setMessage(message);
}

void StockDetailDialog::applyIndicatorOptions()
{
    m_chart->setShowMA(m_cbMA->isChecked());
    m_chart->setShowBollinger(m_cbBB->isChecked());
    m_chart->setShowVolume(m_cbVol->isChecked());
    m_chart->setShowMACD(m_cbMACD->isChecked());
    m_chart->setShowRSI(m_cbRSI->isChecked());
    m_chart->setShowStochastic(m_cbStoch->isChecked());
    saveState();
}

// ── 상태 저장 (INI의 [DetailWindow]) ──

void StockDetailDialog::restoreState()
{
    QSettings s(AppSettings::iniPath(), QSettings::IniFormat);
    s.beginGroup(QStringLiteral("DetailWindow"));
    m_timeframe = s.value(QStringLiteral("Timeframe"), QStringLiteral("day")).toString();
    if (m_timeframe != QLatin1String("week") && m_timeframe != QLatin1String("month"))
        m_timeframe = QStringLiteral("day");

    const QList<QPair<QCheckBox *, QPair<QString, bool>>> boxes = {
        {m_cbMA, {QStringLiteral("MA"), true}},       {m_cbBB, {QStringLiteral("Bollinger"), false}},
        {m_cbVol, {QStringLiteral("Volume"), true}},  {m_cbMACD, {QStringLiteral("MACD"), true}},
        {m_cbRSI, {QStringLiteral("RSI"), true}},     {m_cbStoch, {QStringLiteral("Stochastic"), false}},
    };
    for (const auto &b : boxes) {
        const QSignalBlocker block(b.first);
        b.first->setChecked(s.value(b.second.first, b.second.second).toBool());
    }

    // 창 위치/크기 (화면 밖이면 기본값)
    const QRect r(s.value(QStringLiteral("X"), -1).toInt(), s.value(QStringLiteral("Y"), -1).toInt(),
                  s.value(QStringLiteral("Width"), 1000).toInt(), s.value(QStringLiteral("Height"), 760).toInt());
    const bool maximized = s.value(QStringLiteral("Maximized"), false).toBool();
    s.endGroup();

    bool onScreen = false;
    for (QScreen *scr : QGuiApplication::screens()) {
        if (scr->availableGeometry().intersected(QRect(r.left(), r.top(), r.width(), 40)).width() >= 100)
            onScreen = true;
    }
    if (r.x() >= -10000 && s.contains(QStringLiteral("DetailWindow/X")) && onScreen)
        setGeometry(r);
    else
        resize(qMax(700, r.width()), qMax(500, r.height()));
    if (maximized)
        setWindowState(windowState() | Qt::WindowMaximized);

    applyIndicatorOptions();
}

void StockDetailDialog::saveState() const
{
    QSettings s(AppSettings::iniPath(), QSettings::IniFormat);
    s.beginGroup(QStringLiteral("DetailWindow"));
    s.setValue(QStringLiteral("Timeframe"), m_timeframe);
    s.setValue(QStringLiteral("MA"), m_cbMA->isChecked());
    s.setValue(QStringLiteral("Bollinger"), m_cbBB->isChecked());
    s.setValue(QStringLiteral("Volume"), m_cbVol->isChecked());
    s.setValue(QStringLiteral("MACD"), m_cbMACD->isChecked());
    s.setValue(QStringLiteral("RSI"), m_cbRSI->isChecked());
    s.setValue(QStringLiteral("Stochastic"), m_cbStoch->isChecked());
    s.endGroup();
}

void StockDetailDialog::hideEvent(QHideEvent *e)
{
    QSettings s(AppSettings::iniPath(), QSettings::IniFormat);
    s.beginGroup(QStringLiteral("DetailWindow"));
    const bool maximized = isMaximized();
    const QRect g = maximized ? normalGeometry() : geometry();
    s.setValue(QStringLiteral("X"), g.x());
    s.setValue(QStringLiteral("Y"), g.y());
    s.setValue(QStringLiteral("Width"), g.width());
    s.setValue(QStringLiteral("Height"), g.height());
    s.setValue(QStringLiteral("Maximized"), maximized);
    s.endGroup();
    saveState();
    QDialog::hideEvent(e);
}

void StockDetailDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    if (e->type() == QEvent::PaletteChange)
        renderHeader(); // 스킨이 바뀌면 글자 색 다시 계산
}
