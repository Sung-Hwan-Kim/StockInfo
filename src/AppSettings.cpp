#include "AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace {

bool isDirWritable(const QString &dirPath)
{
    QFile probe(QDir(dirPath).filePath(QStringLiteral(".stockinfo_write_test")));
    if (!probe.open(QIODevice::WriteOnly))
        return false;
    probe.close();
    probe.remove();
    return true;
}

QList<StockEntry> defaultStocks()
{
    return {
        {QStringLiteral("005930"), QStringLiteral("삼성전자"), QStringLiteral("KOSPI")},
        {QStringLiteral("000660"), QStringLiteral("SK하이닉스"), QStringLiteral("KOSPI")},
        {QStringLiteral("247540"), QStringLiteral("에코프로비엠"), QStringLiteral("KOSDAQ")},
        {QStringLiteral("196170"), QStringLiteral("알테오젠"), QStringLiteral("KOSDAQ")},
    };
}

} // namespace

QString AppSettings::iniPath()
{
    static QString cached;
    if (!cached.isEmpty())
        return cached;

    const QString fileName = QStringLiteral("StockInfo.ini");

    // 1순위: 실행 파일과 같은 폴더 (휴대용). macOS 번들 내부는 제외.
#ifndef Q_OS_MACOS
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString exeIni = QDir(exeDir).filePath(fileName);
    if (QFileInfo::exists(exeIni) || isDirWritable(exeDir)) {
        cached = exeIni;
        return cached;
    }
#endif

    // 2순위: 사용자 설정 폴더 (설치형, 쓰기 불가 폴더인 경우)
    QString cfgDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(cfgDir);
    cached = QDir(cfgDir).filePath(fileName);
    return cached;
}

void AppSettings::load()
{
    QSettings s(iniPath(), QSettings::IniFormat);

    s.beginGroup(QStringLiteral("Settings"));
    refreshSeconds = qBound(kMinRefresh, s.value(QStringLiteral("RefreshSeconds"), 5).toInt(), kMaxRefresh);
    listFontSize = qBound(8, s.value(QStringLiteral("ListFontSize"), 11).toInt(), 32);
    alwaysOnTop = s.value(QStringLiteral("AlwaysOnTop"), false).toBool();
    showTrayIcon = s.value(QStringLiteral("TrayIcon"), true).toBool();
    closeToTray = s.value(QStringLiteral("CloseToTray"), true).toBool();
    minimizeToTray = s.value(QStringLiteral("MinimizeToTray"), false).toBool();
    trayNoticeShown = s.value(QStringLiteral("TrayNoticeShown"), false).toBool();
    skin = s.value(QStringLiteral("Skin"), QStringLiteral("Dark")).toString();
    if (skin.compare(QLatin1String("Light"), Qt::CaseInsensitive) == 0)
        skin = QStringLiteral("Light");
    else
        skin = QStringLiteral("Dark");
    s.endGroup();

    s.beginGroup(QStringLiteral("Window"));
    if (s.contains(QStringLiteral("Width")) && s.contains(QStringLiteral("Height"))) {
        windowRect = QRect(s.value(QStringLiteral("X"), 0).toInt(),
                           s.value(QStringLiteral("Y"), 0).toInt(),
                           s.value(QStringLiteral("Width"), 0).toInt(),
                           s.value(QStringLiteral("Height"), 0).toInt());
    } else {
        windowRect = QRect();
    }
    windowMaximized = s.value(QStringLiteral("Maximized"), false).toBool();
    s.endGroup();

    stocks.clear();
    const bool hasStockSection = s.childGroups().contains(QStringLiteral("Stocks"));
    const int count = s.beginReadArray(QStringLiteral("Stocks"));
    for (int i = 0; i < count; ++i) {
        s.setArrayIndex(i);
        StockEntry e;
        e.code = s.value(QStringLiteral("Code")).toString().trimmed().toUpper();
        e.name = s.value(QStringLiteral("Name")).toString();
        e.market = s.value(QStringLiteral("Market"), QStringLiteral("KOSPI")).toString().toUpper();
        if (e.market != QLatin1String("KOSDAQ"))
            e.market = QStringLiteral("KOSPI");
        if (!e.code.isEmpty())
            stocks.append(e);
    }
    s.endArray();

    // 처음 실행 시에만 예시 종목 등록 (사용자가 모두 지운 경우는 그대로 둠)
    if (!hasStockSection)
        stocks = defaultStocks();
}

void AppSettings::save() const
{
    QSettings s(iniPath(), QSettings::IniFormat);

    s.beginGroup(QStringLiteral("Settings"));
    s.setValue(QStringLiteral("RefreshSeconds"), refreshSeconds);
    s.setValue(QStringLiteral("ListFontSize"), listFontSize);
    s.setValue(QStringLiteral("AlwaysOnTop"), alwaysOnTop);
    s.setValue(QStringLiteral("Skin"), skin);
    s.setValue(QStringLiteral("TrayIcon"), showTrayIcon);
    s.setValue(QStringLiteral("CloseToTray"), closeToTray);
    s.setValue(QStringLiteral("MinimizeToTray"), minimizeToTray);
    s.setValue(QStringLiteral("TrayNoticeShown"), trayNoticeShown);
    s.endGroup();

    s.beginGroup(QStringLiteral("Window"));
    if (windowRect.isValid()) {
        s.setValue(QStringLiteral("X"), windowRect.x());
        s.setValue(QStringLiteral("Y"), windowRect.y());
        s.setValue(QStringLiteral("Width"), windowRect.width());
        s.setValue(QStringLiteral("Height"), windowRect.height());
    }
    s.setValue(QStringLiteral("Maximized"), windowMaximized);
    s.endGroup();

    s.remove(QStringLiteral("Stocks"));
    s.beginWriteArray(QStringLiteral("Stocks"), int(stocks.size()));
    for (int i = 0; i < stocks.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue(QStringLiteral("Code"), stocks[i].code);
        s.setValue(QStringLiteral("Name"), stocks[i].name);
        s.setValue(QStringLiteral("Market"), stocks[i].market);
    }
    s.endArray();

    s.sync();
}
