#include <QApplication>
#include "EditorWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("kijitabu");
    QApplication::setOrganizationName("");

    EditorWindow window;
    window.show();

    return app.exec();
}
