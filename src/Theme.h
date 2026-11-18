#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

// 한국식 색상 규칙: 상승 빨강, 하락 파랑
namespace Theme {

inline bool isDark(const QPalette &pal)
{
    return pal.color(QPalette::Window).lightness() < 128;
}

inline QColor directionColor(const QPalette &pal, int direction)
{
    const bool dark = isDark(pal);
    if (direction > 0)
        return dark ? QColor(255, 95, 95) : QColor(214, 30, 30);
    if (direction < 0)
        return dark ? QColor(100, 160, 255) : QColor(20, 80, 210);
    return pal.color(QPalette::Text);
}

inline QColor dimText(const QPalette &pal)
{
    QColor c = pal.color(QPalette::WindowText);
    c.setAlphaF(0.62f);
    return c;
}

inline QString arrow(int direction)
{
    if (direction > 0)
        return QStringLiteral("▲");
    if (direction < 0)
        return QStringLiteral("▼");
    return QString();
}

// "▲ 1,500" / "▼ 300" / "0"
inline QString arrowText(int direction, const QString &value)
{
    const QString a = arrow(direction);
    return a.isEmpty() ? value : a + QLatin1Char(' ') + value;
}

} // namespace Theme
