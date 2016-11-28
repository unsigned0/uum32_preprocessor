#include <QApplication>
#include <QLocale>
#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow mainWin;

    mainWin.setFixedSize(mainWin.size());
    mainWin.show();

    return app.exec();
}
