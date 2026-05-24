#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("2048");
    app.setOrganizationName("CMPE230");

    MainWindow window;
    window.show();

    return app.exec();
}
