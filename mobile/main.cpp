#include "AppController.h"

#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("StockInfo"));
    QGuiApplication::setOrganizationName(QStringLiteral("StockInfo"));
    QGuiApplication::setApplicationVersion(QStringLiteral(STOCKINFO_VERSION));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/icon_256.png")));

#ifdef Q_OS_WIN
    // PC 미리보기: 한글 글꼴 (안드로이드는 시스템 글꼴 사용)
    QFont f = QGuiApplication::font();
    f.setFamilies({QStringLiteral("Malgun Gothic"), f.family()});
    QGuiApplication::setFont(f);
#endif

    QQuickStyle::setStyle(QStringLiteral("Material"));

    AppController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("App"), &controller);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("StockInfo", "Main");
    return app.exec();
}
