#pragma once

#include <QList>
#include <QRect>
#include <QString>

// 관심 종목 한 개
struct StockEntry
{
    QString code;    // 종목코드 (예: 005930)
    QString name;    // 종목명 (예: 삼성전자)
    QString market;  // "KOSPI" 또는 "KOSDAQ"
};

// 환경설정 (INI 파일에 저장)
class AppSettings
{
public:
    // 갱신 주기 (초)
    int refreshSeconds = 5;
    // 종목 목록 글꼴 크기 (pt)
    int listFontSize = 11;
    // 항상 위에 표시
    bool alwaysOnTop = false;
    // 스킨: "Dark"(기본) / "Light"
    QString skin = QStringLiteral("Dark");

    // 트레이
    bool showTrayIcon = true;     // 알림 영역(트레이)에 아이콘 표시
    bool closeToTray = true;      // 닫기(X) 버튼 → 트레이로 숨기기
    bool minimizeToTray = false;  // 최소화 → 트레이로 숨기기
    bool trayNoticeShown = false; // "트레이에서 실행 중" 안내를 이미 보여줬는지

    // 창 위치/크기 (클라이언트 영역 기준), 비어 있으면 기본 위치
    QRect windowRect;
    bool windowMaximized = false;

    QList<StockEntry> stocks;

    static constexpr int kMinRefresh = 1;
    static constexpr int kMaxRefresh = 3600;

    void load();
    void save() const;

    // 실제로 사용 중인 INI 파일 경로
    static QString iniPath();
};
