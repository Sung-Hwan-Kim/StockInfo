#include "MainWindow.h"
#include "Skin.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFont>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStyleFactory>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {

// 사용자별 중복 실행 확인용 이름 (같은 PC의 다른 사용자와는 겹치지 않게)
QString instanceKey()
{
    const QByteArray hash = QCryptographicHash::hash(QDir::homePath().toUtf8(), QCryptographicHash::Sha1).toHex();
    return QStringLiteral("StockInfo-%1").arg(QString::fromLatin1(hash.left(16)));
}

// 이미 실행 중인 StockInfo가 있으면 그 창을 띄우라고 알리고 true
bool notifyRunningInstance(const QString &key)
{
    QLocalSocket socket;
    socket.connectToServer(key);
    if (!socket.waitForConnected(500))
        return false;
#ifdef Q_OS_WIN
    // 기존 프로그램이 창을 맨 앞으로 가져올 수 있게 허용
    AllowSetForegroundWindow(ASFW_ANY);
#endif
    socket.write("show\n");
    socket.waitForBytesWritten(500);
    socket.disconnectFromServer();
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("StockInfo"));
    QApplication::setOrganizationName(QStringLiteral("StockInfo"));
    QApplication::setApplicationVersion(QStringLiteral(STOCKINFO_VERSION));
    // 창을 트레이로 숨겨도 계속 실행 (종료는 [종료] 메뉴에서)
    QApplication::setQuitOnLastWindowClosed(false);

    // 중복 실행 방지: 이미 실행 중이면 기존 창을 띄우고 끝낸다
    const QString key = instanceKey();
    if (notifyRunningInstance(key))
        return 0;

    // 스킨이 OS 테마와 상관없이 똑같이 보이도록 Fusion 스타일 사용
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    Skin::installWindowFrameFilter();

    // 창/작업 표시줄/트레이 아이콘
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addFile(QStringLiteral(":/resources/icons/icon_%1.png").arg(size), QSize(size, size));
    QApplication::setWindowIcon(icon);

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

    // 나중에 실행된 StockInfo의 "창 띄우기" 요청을 받는다
    QLocalServer server;
    server.setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(key); // 비정상 종료로 남은 소켓 정리 (Unix)
    if (server.listen(key)) {
        QObject::connect(&server, &QLocalServer::newConnection, &w, [&server, &w] {
            while (QLocalSocket *client = server.nextPendingConnection()) {
                QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
                w.showFromTray();
            }
        });
    }

    w.restoreAndShow();
    return app.exec();
}
