#include "directorydialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DirectoryDialog w;
    w.exec();
    return a.exec();
}
