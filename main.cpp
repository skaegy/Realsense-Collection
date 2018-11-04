#include "collectdata.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    rsCollectData w;
    w.show();

    return a.exec();
}
