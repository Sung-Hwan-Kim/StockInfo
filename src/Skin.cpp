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

#ifdef Q_OS_MACOS
void applyMacAppearance(bool dark); // SkinMac.mm
#endif

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

#ifdef Q_OS_MACOS
    // macOS: 제목 표시줄과 시스템 메뉴도 스킨에 맞춘다
    applyMacAppearance(m == Mode::Dark);
#endif
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
