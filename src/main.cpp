#include "MainWindow.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("StockInfo"));
    QApplication::setOrganizationName(QStringLiteral("StockInfo"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    // 플랫폼별 한글 글꼴 우선 지정
    QFont f = QApplication::font();
    QStringList families;
#if defined(Q_OS_WIN)
    families << QStringLiteral("Malgun Gothic") << QStringLiteral("맑은 고딕");
#elif defined(Q_OS_MACOS)
    families << QStringLiteral("Apple SD Gothic Neo");
#else
    families << QStringLiteral("Noto Sans CJK KR") << QStringLiteral("NanumGothic");
#endif
    families << f.family();
    f.setFamilies(families);
    QApplication::setFont(f);

    MainWindow w;
    w.restoreAndShow();
    return app.exec();
}
