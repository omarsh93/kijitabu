#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include "EditorWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("kijitabu");
    QApplication::setOrganizationName("");

    QLocalSocket socket;
    socket.connectToServer("kijitabu");
    if (socket.waitForConnected(1000))
    {
        socket.write("activate");
        socket.waitForBytesWritten(1000);
        return 0;
    }

    QLocalServer::removeServer("kijitabu");

    QLocalServer server;
    server.listen("kijitabu");

    EditorWindow window;
    window.show();

    QObject::connect(&server, &QLocalServer::newConnection, [&]()
    {
        window.bringToFront();
    });

    return app.exec();
}
