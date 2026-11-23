// macOS: 앱 전체 모양(제목 표시줄, 메뉴 막대의 메뉴)을 스킨에 맞춰 다크/라이트로 바꾼다
// (Windows의 DwmSetWindowAttribute에 해당)
#import <AppKit/AppKit.h>

namespace Skin {

void applyMacAppearance(bool dark)
{
    NSAppearanceName name = dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua;
    [NSApp setAppearance:[NSAppearance appearanceNamed:name]];
}

} // namespace Skin
