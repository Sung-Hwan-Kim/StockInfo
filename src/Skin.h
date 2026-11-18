#pragma once

#include <QPalette>
#include <QString>

// 다크/라이트 스킨. OS 테마와 상관없이 같은 모양이 나오도록
// Fusion 스타일 + 직접 정의한 팔레트를 사용한다.
namespace Skin {

enum class Mode { Dark, Light };

Mode fromString(const QString &s);    // 알 수 없는 값이면 Dark (기본값)
QString toString(Mode m);             // INI 저장값: "Dark" / "Light"
QString displayName(Mode m);          // "다크 모드" / "라이트 모드"

QPalette palette(Mode m);

// 앱 전체에 스킨 적용 (팔레트 + Windows 제목 표시줄 색)
void apply(Mode m);
Mode current();

// 새로 뜨는 창(대화상자, 메시지 상자 포함)의 제목 표시줄도 스킨에 맞추도록 이벤트 필터 설치
void installWindowFrameFilter();

} // namespace Skin
