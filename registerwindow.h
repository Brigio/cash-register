#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QTreeView>
#include <QSqlQueryModel>
#include <QTranslator>

#include "aboutdialog.h"

class QPrinter;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE
//=====================================================
//Classe impostazioni Enabled Pulsanti e caselle testo
//=====================================================
enum class EditState {
    Idle,       // Nessuna riga selezionata
    Viewing,    // Una riga è selezionata
    Editing,    // Sto modificando un record
    Creating    // Sto creando un nuovo record
};
//=================================================
//Classe impostazioni Cella Record tabella
//=================================================
enum Col {
    COL_ID = 0,
    COL_DATA = 1,
    COL_IMPORTO = 2,
    COL_SALDO = 3,
    COL_GRUPPO = 4,
    COL_NOTE = 5
};
//Questo dice al compilatore:
//"La variabile esiste da qualche parte, non creare qui una copia"
extern EditState editState; //Dichiarazione vedi ccp
//===============================================
class MainWindow : public QMainWindow
{
    Q_OBJECT

private slots:
    void onGroupsChanged();

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void populateTreeView(bool showByGroup = false);
    void elaboraPeriodo(QString gruppo, QString anno, int mese);
    void aggiornaEntrateUscite();
    void aggiornaPulsanti();
    void selezionaTreeView(QString gruppo, QString anno, int mese);
    double calcolaTotaleCassa();

private slots:
    void on_pubTreeview_toggled(bool checked);
    void on_pubSync_clicked();
    double calcolaSaldoPrecedente(QString gruppo, QString anno, int mese);
    void on_pubGroups_clicked();
    void on_btnNuovo_clicked();
    void on_tabMovements_clicked(const QModelIndex &index);
    void on_btnDelete_clicked();
    void on_btnAnnulla_clicked();
    void aggiornaTitoloGroupBox();
    void on_btnSalva_clicked();
    void on_btnModifica_clicked();
    void on_btnElimina_clicked();
    void on_btnStampa_clicked();
    void stampaMovimenti(QPrinter &printer);
    void aggiornaTotaleCassa();
    void aggiornaTesti();
    void aggiornaIntestazioniTabella();
    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;
    QSqlDatabase sqlitedb;
    QStandardItemModel *treeModel;
    QSqlQueryModel *tableModel;
    bool inizializzaDatabase();
    int currentRecordId = -1;
    QTranslator translator;
    void impostaLingua(const QString &codice);
};

#endif // REGISTERWINDOW_H
