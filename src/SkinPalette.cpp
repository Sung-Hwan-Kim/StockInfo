#include "Skin.h"

// 스킨 색 정의 (QtGui만 사용 — PC 위젯 앱과 모바일 QML 앱이 같이 쓴다)
namespace Skin {

Mode fromString(const QString &s)
{
    return s.compare(QLatin1String("Light"), Qt::CaseInsensitive) == 0 ? Mode::Light : Mode::Dark;
}

QString toString(Mode m)
{
    return m == Mode::Light ? QStringLiteral("Light") : QStringLiteral("Dark");
}

QString displayName(Mode m)
{
    return m == Mode::Light ? QStringLiteral("라이트 모드") : QStringLiteral("다크 모드");
}

QPalette palette(Mode m)
{
    QPalette p;
    auto set = [&p](QPalette::ColorRole role, const QColor &c) { p.setColor(QPalette::All, role, c); };
    auto disabled = [&p](QPalette::ColorRole role, const QColor &c) { p.setColor(QPalette::Disabled, role, c); };

    if (m == Mode::Dark) {
        set(QPalette::Window, QColor(0x1e, 0x1f, 0x22));
        set(QPalette::WindowText, QColor(0xe6, 0xe7, 0xea));
        set(QPalette::Base, QColor(0x25, 0x27, 0x2b));
        set(QPalette::AlternateBase, QColor(0x2c, 0x2f, 0x34));
        set(QPalette::Text, QColor(0xe6, 0xe7, 0xea));
        set(QPalette::Button, QColor(0x2d, 0x30, 0x35));
        set(QPalette::ButtonText, QColor(0xe6, 0xe7, 0xea));
        set(QPalette::BrightText, QColor(0xff, 0x6b, 0x6b));
        set(QPalette::Highlight, QColor(0x3a, 0x6e, 0xc4));
        set(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        set(QPalette::ToolTipBase, QColor(0x2d, 0x30, 0x35));
        set(QPalette::ToolTipText, QColor(0xe6, 0xe7, 0xea));
        set(QPalette::PlaceholderText, QColor(0x8a, 0x8f, 0x98));
        set(QPalette::Link, QColor(0x6e, 0xa8, 0xfe));
        set(QPalette::LinkVisited, QColor(0xb0, 0x8c, 0xf5));
        set(QPalette::Light, QColor(0x45, 0x48, 0x4e));
        set(QPalette::Midlight, QColor(0x37, 0x3a, 0x40));
        set(QPalette::Mid, QColor(0x2a, 0x2c, 0x30));
        set(QPalette::Dark, QColor(0x16, 0x17, 0x19));
        set(QPalette::Shadow, QColor(0x00, 0x00, 0x00));
        const QColor dis(0x6b, 0x6f, 0x76);
        disabled(QPalette::WindowText, dis);
        disabled(QPalette::Text, dis);
        disabled(QPalette::ButtonText, dis);
        disabled(QPalette::Highlight, QColor(0x3a, 0x3d, 0x42));
    } else {
        set(QPalette::Window, QColor(0xf3, 0xf4, 0xf6));
        set(QPalette::WindowText, QColor(0x1b, 0x1c, 0x1f));
        set(QPalette::Base, QColor(0xff, 0xff, 0xff));
        set(QPalette::AlternateBase, QColor(0xf4, 0xf6, 0xf9));
        set(QPalette::Text, QColor(0x1b, 0x1c, 0x1f));
        set(QPalette::Button, QColor(0xea, 0xec, 0xef));
        set(QPalette::ButtonText, QColor(0x1b, 0x1c, 0x1f));
        set(QPalette::BrightText, QColor(0xd6, 0x1e, 0x1e));
        set(QPalette::Highlight, QColor(0x2f, 0x6f, 0xde));
        set(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        set(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xff));
        set(QPalette::ToolTipText, QColor(0x1b, 0x1c, 0x1f));
        set(QPalette::PlaceholderText, QColor(0x8a, 0x8f, 0x98));
        set(QPalette::Link, QColor(0x1f, 0x5f, 0xc8));
        set(QPalette::LinkVisited, QColor(0x6f, 0x42, 0xc1));
        set(QPalette::Light, QColor(0xff, 0xff, 0xff));
        set(QPalette::Midlight, QColor(0xe3, 0xe5, 0xe9));
        set(QPalette::Mid, QColor(0xc3, 0xc6, 0xcc));
        set(QPalette::Dark, QColor(0xa0, 0xa4, 0xab));
        set(QPalette::Shadow, QColor(0x6b, 0x6f, 0x76));
        const QColor dis(0xa0, 0xa4, 0xab);
        disabled(QPalette::WindowText, dis);
        disabled(QPalette::Text, dis);
        disabled(QPalette::ButtonText, dis);
        disabled(QPalette::Highlight, QColor(0xd0, 0xd3, 0xd8));
    }
    return p;
}

} // namespace Skin
