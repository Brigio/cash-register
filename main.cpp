#include "registerwindow.h"
#include <QApplication>

bool inizializzaDatabase();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Franco");
    QCoreApplication::setApplicationName("Cash_Register");

    MainWindow w;
    w.show();
    return a.exec();
}
