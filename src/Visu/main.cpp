#include "EmbSysVisu.h"
#include "connectionuart.h"
#include <QApplication>

int main( int argc, char* argv[])
{
        QApplication a(argc, argv);
        EmbSysVisu EmbSys;
        EmbSys.show();
        return a.exec();
}
