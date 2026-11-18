#include "Skin.h"

#include <QApplication>
#include <QEvent>
#include <QTimer>
#include <QToolTip>
#include <QWidget>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <dwmapi.h>
#endif

namespace Skin {

namespace {

Mode g_mode = Mode::Dark;

// Windows 10(1809+)/11: 제목 표시줄을 다크/라이트로 맞춘다
void applyFrame(QWidget *w, bool forceRedraw)
{
#ifdef Q_OS_WIN
    if (!w || !w->isWindow())
        return;
    const Qt::WindowType type = w->windowType();
    if (type != Qt::Window && type != Qt::Dialog)
        return; // 메뉴, 툴팁 등은 제외

    const HWND hwnd = reinterpret_cast<HWND>(w->winId());
    const BOOL dark = (g_mode == Mode::Dark) ? TRUE : FALSE;
    constexpr DWORD kUseImmersiveDarkMode = 20;       // DWMWA_USE_IMMERSIVE_DARK_MODE
    constexpr DWORD kUseImmersiveDarkModeOld = 19;    // Windows 10 1809~1909
    if (FAILED(DwmSetWindowAttribute(hwnd, kUseImmersiveDarkMode, &dark, sizeof dark)))
        DwmSetWindowAttribute(hwnd, kUseImmersiveDarkModeOld, &dark, sizeof dark);

    if (forceRedraw && w->isVisible()) {
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
#else
    Q_UNUSED(w);
    Q_UNUSED(forceRedraw);
#endif
}

class FrameFilter : public QObject
{
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj->isWidgetType()) {
            auto *w = static_cast<QWidget *>(obj);
            if (ev->type() == QEvent::Show) {
                applyFrame(w, false);
            } else if (ev->type() == QEvent::ThemeChange && w->isWindow()) {
                // OS 테마가 바뀌면 Qt가 제목 표시줄을 OS 기준으로 되돌리므로 다시 적용
                QTimer::singleShot(0, w, [w] { applyFrame(w, true); });
            }
        }
        return QObject::eventFilter(obj, ev);
    }
};

} // namespace

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

void apply(Mode m)
{
    g_mode = m;
    const QPalette pal = palette(m);
    // 전역 팔레트를 설정하면 위젯별 팔레트는 초기화된다 (그 뒤에 개별 지정)
    QApplication::setPalette(pal);
    QToolTip::setPalette(pal);

    if (m == Mode::Dark) {
        // Fusion은 체크박스/라디오 테두리를 Window 색을 어둡게 해서 그리므로
        // 다크 모드에서는 테두리가 배경에 묻힌다 → 해당 위젯만 대비를 높인다
        QPalette indicator = pal;
        indicator.setColor(QPalette::Window, QColor(0x9a, 0x9f, 0xa8));
        indicator.setColor(QPalette::Base, QColor(0x30, 0x33, 0x38));
        QApplication::setPalette(indicator, "QCheckBox");
        QApplication::setPalette(indicator, "QRadioButton");
    }

    const auto windows = QApplication::topLevelWidgets();
    for (QWidget *w : windows) {
        if (w->isVisible())
            applyFrame(w, true);
    }
}

Mode current()
{
    return g_mode;
}

void installWindowFrameFilter()
{
    static FrameFilter *filter = nullptr;
    if (!filter) {
        filter = new FrameFilter(qApp);
        qApp->installEventFilter(filter);
    }
}

} // namespace Skin
