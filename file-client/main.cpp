#include "dlg.h"
#include "header.h"
#include <QApplication>
#include "sha256.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dlg w;
    w.show();
    return a.exec();
}
