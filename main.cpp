#include <QApplication>
#include <QCoreApplication>
#include "registerwindow.h"

bool inizializzaDatabase();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Brigio");
    QCoreApplication::setApplicationName("cash-register");

    MainWindow w;
    w.show();
    return a.exec();
}
