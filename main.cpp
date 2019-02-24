#include "mainwindow.h"
#include <QApplication>

void showUsage()
{
    printf("Usage: DemoTest w h");
    printf("    THe width/height of media in file to be opened must be specified correctly!\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (argc < 3) {
        showUsage();
        return;
    }

    int width  = QString(argv[1]).toInt();
    int height = QString(argv[2]).toInt();
    if (width <= 0 || height <= 0) {
        showUsage();
        return;
    }

    MainWindow w(width, height);
    w.show();

    return a.exec();
}
