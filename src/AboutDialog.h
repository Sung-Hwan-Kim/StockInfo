#pragma once

#include <QDialog>

// 도움말 → 버전 정보
class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);

    static QString version();      // 예: "1.2.0"
    static QString buildDate();    // 예: "2026-11-18"
    static QString homepage();     // GitHub 저장소 주소
    // 버그 신고 등에 붙여 넣기 좋은 한 덩어리 텍스트
    static QString versionText();
};
