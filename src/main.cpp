#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Assignment 3 Spring 26 Template");
    app.setOrganizationName("CMPE230");

    MainWindow window;
    window.show();

    return app.exec();
}
