#include "AboutDialog.h"

#include "AppSettings.h"
#include "Theme.h"

#include <QApplication>
#include <QClipboard>
#include <QDate>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSysInfo>
#include <QVBoxLayout>

#ifndef STOCKINFO_VERSION
#define STOCKINFO_VERSION "0.0.0"
#endif

namespace {

QString compilerName()
{
#if defined(_MSC_VER)
    return QStringLiteral("MSVC %1.%2").arg(_MSC_VER / 100).arg(_MSC_VER % 100);
#elif defined(__clang__)
    return QStringLiteral("Clang %1.%2.%3").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__);
#elif defined(__GNUC__)
    return QStringLiteral("GCC %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__);
#else
    return QStringLiteral("알 수 없음");
#endif
}

QString qtVersionText()
{
    const QString runtime = QString::fromLatin1(qVersion());
    const QString built = QStringLiteral(QT_VERSION_STR);
    return runtime == built ? runtime : QStringLiteral("%1 (빌드: %2)").arg(runtime, built);
}

QLabel *selectable(const QString &text, QWidget *parent)
{
    auto *l = new QLabel(text, parent);
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    l->setWordWrap(true);
    return l;
}

} // namespace

QString AboutDialog::version()
{
    return QStringLiteral(STOCKINFO_VERSION);
}

QString AboutDialog::buildDate()
{
    // __DATE__ 형식: "Nov 18 2026" (한 자리 날짜는 "Nov  8 2026")
    const QString raw = QString::fromLatin1(__DATE__).simplified();
    const QDate d = QLocale::c().toDate(raw, QStringLiteral("MMM d yyyy"));
    return d.isValid() ? d.toString(Qt::ISODate) : raw;
}

QString AboutDialog::homepage()
{
    return QStringLiteral("https://github.com/Sung-Hwan-Kim/StockInfo");
}

QString AboutDialog::versionText()
{
    return QStringLiteral("StockInfo %1 (빌드 %2)\nQt %3, %4\n%5 (%6)")
        .arg(version(), buildDate(), qtVersionText(), compilerName(),
             QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
}

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("버전 정보"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // ── 머리: 아이콘 + 이름 + 버전 ──
    auto *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(QStringLiteral(":/resources/icons/icon_128.png")).pixmap(80, 80));
    iconLabel->setAlignment(Qt::AlignTop);

    auto *title = new QLabel(QStringLiteral("StockInfo"), this);
    QFont tf = title->font();
    tf.setPointSizeF(tf.pointSizeF() * 1.9);
    tf.setBold(true);
    title->setFont(tf);

    auto *subtitle = new QLabel(tr("코스피 · 코스닥 주식 정보"), this);
    QFont sf = subtitle->font();
    sf.setPointSizeF(sf.pointSizeF() * 1.15);
    subtitle->setFont(sf);

    auto *versionLabel = new QLabel(tr("버전 %1").arg(version()), this);
    QFont vf = versionLabel->font();
    vf.setPointSizeF(vf.pointSizeF() * 1.15);
    vf.setBold(true);
    versionLabel->setFont(vf);
    QPalette vp = versionLabel->palette();
    vp.setColor(QPalette::WindowText, palette().color(QPalette::Link));
    versionLabel->setPalette(vp);

    auto *headText = new QVBoxLayout;
    headText->setSpacing(2);
    headText->addWidget(title);
    headText->addWidget(subtitle);
    headText->addSpacing(4);
    headText->addWidget(versionLabel);
    headText->addStretch();

    auto *head = new QHBoxLayout;
    head->setSpacing(16);
    head->addWidget(iconLabel);
    head->addLayout(headText, 1);

    // ── 상세 정보 ──
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->setHorizontalSpacing(12);
    form->addRow(tr("빌드 날짜:"), selectable(buildDate(), this));
    form->addRow(tr("Qt 버전:"), selectable(qtVersionText(), this));
    form->addRow(tr("컴파일러:"), selectable(compilerName(), this));
    form->addRow(tr("운영체제:"), selectable(QStringLiteral("%1 (%2)").arg(QSysInfo::prettyProductName(),
                                                                       QSysInfo::currentCpuArchitecture()), this));
    form->addRow(tr("설정 파일:"), selectable(QDir::toNativeSeparators(AppSettings::iniPath()), this));
    form->addRow(tr("시세 출처:"), selectable(tr("네이버 금융 (비공식 공개 API)"), this));

    auto *link = new QLabel(QStringLiteral("<a href=\"%1\">%1</a>").arg(homepage()), this);
    link->setOpenExternalLinks(true);
    link->setTextInteractionFlags(Qt::TextBrowserInteraction);
    form->addRow(tr("홈페이지:"), link);

    auto *notice = new QLabel(tr("표시되는 시세는 참고용이며 실제 시세와 차이가 있을 수 있습니다."), this);
    notice->setWordWrap(true);
    QPalette np = notice->palette();
    np.setColor(QPalette::WindowText, Theme::dimText(palette()));
    notice->setPalette(np);

    // ── 버튼 ──
    auto *buttons = new QDialogButtonBox(this);
    QPushButton *copyBtn = buttons->addButton(tr("버전 정보 복사"), QDialogButtonBox::ActionRole);
    QPushButton *closeBtn = buttons->addButton(tr("닫기"), QDialogButtonBox::AcceptRole);
    closeBtn->setDefault(true);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(copyBtn, &QPushButton::clicked, this, [copyBtn] {
        QApplication::clipboard()->setText(versionText());
        copyBtn->setText(tr("복사됨"));
    });

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 14);
    root->setSpacing(14);
    root->addLayout(head);
    root->addLayout(form);
    root->addWidget(notice);
    root->addWidget(buttons);

    setMinimumWidth(460);
}
