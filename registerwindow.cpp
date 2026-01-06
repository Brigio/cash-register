#include "registerwindow.h"
#include "./ui_registerwindow.h"
#include "coloredelegate.h"
#include "groupswindow.h"
#include "aboutdialog.h"

#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QFontMetrics>

#include <QTranslator>

#include <QStandardPaths>
#include <QDir>
#include <QSqlError>

//Controllo del formato euro lndImporto
#include <QRegularExpressionValidator>

//Dichiarazione del Modello e del QTreeView
//NON NECESSARI treeModel è già membro di MainWindow, trvDate è ui->trvDate
//----QTreeView *trvDate;
//----QStandardItemModel *treeModel;

//definizione vera editstate della classe EditState
EditState editState = EditState::Idle;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)


{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/icons/app_logo.png"));

    // ===============================
    // Icone standard Qt (UI)
    // ===============================
    ui->btnNuovo->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    ui->btnSalva->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    ui->btnAnnulla->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    ui->btnElimina->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    ui->btnModifica->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    ui->btnStampa->setIcon(QIcon::fromTheme("document-print", style()->standardIcon(QStyle::SP_FileIcon)));

    // ===============================
    // Lingua all'avvio
    // ===============================
    QSettings settings;
    QString lang = settings.value("language", "it").toString();
    impostaLingua(lang);

    // ===============================
    // Menu Lingua - icone
    // ===============================
    ui->actionItaliano->setIcon(QIcon(":/icons/italiano.png"));
    ui->actionInglese->setIcon(QIcon(":/icons/inglese.png"));

    //*******************************************************************************
    //**********  MENU LINGUA it en  ************************************************
    connect(ui->actionItaliano, &QAction::triggered, this, [this]() {
        impostaLingua("it");
    });

    connect(ui->actionInglese, &QAction::triggered, this, [this]() {
        impostaLingua("en");
    });
    //*******************************************************************************
    //*******************************************************************************

    //*******************************************************************************
    //*****SALVATAGIO IMPOSTAZIONI SALDO INIZIALE******************************************
    // Stato iniziale label
    ui->lblMovPrecedenti->setVisible(false);

    // Ripristino impostazione utente
    //QSettings settings; già dichiarato Lingua all'avvio
    bool movPrec = settings.value("chkMovPrecedenti", false).toBool();
    ui->chkMovPrecedenti->setChecked(movPrec);
    ui->lblMovPrecedenti->setVisible(movPrec);

    // Gestione visibilità label
    connect(ui->chkMovPrecedenti, &QCheckBox::toggled,
            this, [&](bool checked){
                ui->lblMovPrecedenti->setVisible(checked);
            });

    // Salvataggio impostazione
    connect(ui->chkMovPrecedenti, &QCheckBox::toggled,
            this, [](bool checked){
                QSettings settings;
                settings.setValue("chkMovPrecedenti", checked);
            });
    //*******************************************************************************
    //*******************************************************************************

    aggiornaTotaleCassa();

    //Impostazioni Iniziali pulsanti databse
    editState = EditState::Idle;
    aggiornaPulsanti();

    //impostazione iniziale pulsante TreeView
    ui->pubTreeview->setIcon(QIcon(":/icons/group.png"));
    ui->pubTreeview->setText(tr("Vista Gruppo"));

    //Pulsante Sincronismo e cancellazione database
    ui->pubSync->setIcon(QIcon(":/icons/Sync.png"));
    ui->btnDelete->setIcon(QIcon(":/icons/Delete.png"));

    //Tabella selezione riga singola
    ui->tabMovements->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tabMovements->setSelectionMode(QAbstractItemView::SingleSelection);

    //TreeView non modificabile
    ui->trvDate->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //Query database
    tableModel = new QSqlQueryModel(this);
    ui->tabMovements->setModel(tableModel);

     //Impostazione colore caselle movimeto e saldo nella tabella
    ui->tabMovements->setItemDelegateForColumn(COL_IMPORTO, new ColoreDelegate(this)); // movimento 2
    ui->tabMovements->setItemDelegateForColumn(COL_SALDO, new ColoreDelegate(this)); // saldo 3

    inizializzaDatabase();
    populateTreeView(false);

    //Azzero TreeView per il pulsante btnDelete
    ui->trvDate->clearSelection();
    ui->trvDate->setCurrentIndex(QModelIndex());   //Disattiva qualunque item corrente

    //Controllo del formato euro lndImporto ============================================================
    //Non serve nessun signal: il validator funziona automaticamente appena lo assegno alla QLineEdit.
    QRegularExpression re(R"(^-?\d*([.,]\d{0,2})?$)");
    ui->lndImporto->setValidator(new QRegularExpressionValidator(re, this));

    // --- Sostituzione virgola con punto, titolo e colore box  ---
    connect(ui->lndImporto, &QLineEdit::textEdited, this, [this](QString t){
        t.replace(',', '.');
        ui->lndImporto->setText(t);
    });
    connect(ui->lndImporto, &QLineEdit::textChanged, this, &MainWindow::aggiornaTitoloGroupBox);
    //=================================================================================================
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnNuovo_clicked()
{
    editState = EditState::Creating;
    aggiornaPulsanti();
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_pubSync_clicked()
{
    QModelIndex index = ui->trvDate->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, tr("Attenzione"), tr("Seleziona un elemento."));
        return;
    }

    QStandardItem *item = treeModel->itemFromIndex(index);
    QString tipo = item->data(Qt::UserRole + 1).toString();
    QVariant value = item->data(Qt::UserRole + 2);

    QString gruppo;
    QString anno;
    int mese = 0;

    if (tipo == "group") {
        gruppo = value.toString();

        // Correzione: la sentinella è "__NO_GROUP__"
        if (gruppo == "__NO_GROUP__")
            gruppo = "__NO_GROUP__";   // mantiene la sentinella per le query
    }
    else if (tipo == "year") {
        anno = value.toString();

        // Recupero gruppo se esiste
        if (item->parent())
            gruppo = item->parent()->data(Qt::UserRole + 2).toString();
    }
    else if (tipo == "month") {
        mese = value.toInt();
        QStandardItem *yearItem = item->parent();
        anno = yearItem->data(Qt::UserRole + 2).toString();

        // Solo se vista per gruppi → può esserci un livello sopra
        if (yearItem->parent())
            gruppo = yearItem->parent()->data(Qt::UserRole + 2).toString();
    }

    elaboraPeriodo(gruppo, anno, mese);
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_pubGroups_clicked()
{
    // ------------------------------------
    // 1. Determino la modalità di apertura
    // ------------------------------------
    GroupsWindow::Mode mode;

    if (editState == EditState::Creating ||
        editState == EditState::Editing)
    {
        mode = GroupsWindow::Mode::FullEdit;
    }
    else
    {
        mode = GroupsWindow::Mode::SelectOnly;
    }

    // ------------------------------------
    // 2. Gruppo corrente dal form principale
    // ------------------------------------
    QString currentGroup = ui->lblGroup->text().trimmed();

    // ------------------------------------
    // 3. Creazione finestra Gruppi
    // ------------------------------------
    GroupsWindow win(
        sqlitedb,
        mode,
        currentGroup,
        this
        );

    // ------------------------------------
    // 4. Ritorno gruppo selezionato
    // ------------------------------------
    connect(&win, &GroupsWindow::groupChosen,
            this, [this](const QString &g){
                ui->lblGroup->setText(g);
            });

    // ------------------------------------
    // 5. Collegamento alla funzione groupsChanged
    // ------------------------------------
    connect(&win, &GroupsWindow::groupsChanged,
            this, &MainWindow::onGroupsChanged);

    // ------------------------------------
    // 6. Apertura modale
    // ------------------------------------
    win.exec();
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_pubTreeview_toggled(bool checked)
{
    // Cambia icona in base allo stato
    if (checked) {
        ui->pubTreeview->setIcon(QIcon(":/icons/month.png"));
        ui->pubTreeview->setText(tr("Vista Date"));
    } else {
        ui->pubTreeview->setIcon(QIcon(":/icons/group.png"));
        ui->pubTreeview->setText(tr("Vista Gruppo"));
    }

    //Nelle proprietà del pulsante impostato il flag su checkable
    populateTreeView(checked);
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_tabMovements_clicked(const QModelIndex &index)
{

    if (!index.isValid())
        return;

    // -------------------------------------------------
    // Se stavo creando o modificando, annulla operazione
    // -------------------------------------------------
    if (editState == EditState::Creating || editState == EditState::Editing)
    {
        if (QMessageBox::question(this, tr("Operazione in corso"), tr("Vuoi annullare le modifiche correnti?")) != QMessageBox::Yes)
            return;

        on_btnAnnulla_clicked();
    }

    // ------------------------------------
    // Stato: visualizzazione record
    // ------------------------------------
    editState = EditState::Viewing;
    aggiornaPulsanti();

    int row = index.row();

    // ------------------------------------
    // ID RECORD (necessario per UPDATE)
    // ------------------------------------
    currentRecordId = tableModel->data(tableModel->index(row, COL_ID)).toInt();

    // ------------------------------------
    // DATA
    // ------------------------------------
    QString dataStr = tableModel->data(tableModel->index(row, COL_DATA)).toString();

    QDate data = QDate::fromString(dataStr, "dd/MM/yyyy");
    ui->dateEdit->setDate(data);
    ui->dateEdit->setDisplayFormat("dd/MM/yyyy");

    // ------------------------------------
    // IMPORTO
    // ------------------------------------
    double importo = tableModel->data(tableModel->index(row, COL_IMPORTO)).toDouble();

    ui->lndImporto->setText(QString::number(importo, 'f', 2));

    // Aggiorna titolo GroupBox (Entrata/Uscita)
    aggiornaTitoloGroupBox();

    // ------------------------------------
    // GRUPPO
    // ------------------------------------
    QString gruppo = tableModel->data(tableModel->index(row, COL_GRUPPO)).toString();

    ui->lblGroup->setText(gruppo);

    // ------------------------------------
    // NOTE
    // ------------------------------------
    QString note = tableModel->data(tableModel->index(row, COL_NOTE)).toString();

    ui->lndNote->setText(note);
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_btnDelete_clicked()
{
    QModelIndex index = ui->trvDate->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, tr("Attenzione"), tr("Seleziona un elemento nel TreeView."));
        return;
    }

    QStandardItem* item = treeModel->itemFromIndex(index);
    QString tipo = item->data(Qt::UserRole + 1).toString();
    QVariant value = item->data(Qt::UserRole + 2);

    QString gruppo;
    QString anno;
    int mese = 0;

    // --- Identifica cosa è stato selezionato ---
    if (tipo == "group") {
        gruppo = value.toString();     // "__NO_GROUP__" o nome gruppo
    }
    else if (tipo == "year") {
        anno = value.toString();

        if (item->parent())  // se in vista per group
            gruppo = item->parent()->data(Qt::UserRole + 2).toString();
    }
    else if (tipo == "month") {
        mese = value.toInt();

        QStandardItem* yearItem = item->parent();
        anno = yearItem->data(Qt::UserRole + 2).toString();

        if (yearItem->parent()) // vista gruppo
            gruppo = yearItem->parent()->data(Qt::UserRole + 2).toString();
    }

    // --- Messaggio di conferma ---
    QLocale loc;

    QString msg = tr("Confermi di voler cancellare i movimenti?\n\n");

    if (!gruppo.isEmpty()) {
        msg += tr("Gruppo: %1\n")
        .arg(gruppo == "__NO_GROUP__" ? tr("<Nessun Gruppo>") : gruppo);
    }
    if (!anno.isEmpty()) {
        msg += tr("Anno: %1\n").arg(anno);
    }
    if (mese > 0) {
        msg += tr("Mese: %1\n")
        .arg(loc.monthName(mese, QLocale::LongFormat));
    }

    if (QMessageBox::question(this, tr("Conferma Eliminazione"), msg) != QMessageBox::Yes)
        return;

    // --- Costruzione DELETE SQL ---
    QString sql = "DELETE FROM cassa WHERE 1=1";

    if (gruppo == "__NO_GROUP__") {
        sql += " AND (gruppo IS NULL OR gruppo = '')";
    }
    else if (!gruppo.isEmpty()) {
        sql += " AND gruppo = :gruppo";
    }

    if (!anno.isEmpty())
        sql += " AND strftime('%Y', data) = :anno";

    if (mese > 0)
        sql += " AND strftime('%m', data) = :mese";

    QSqlQuery q(sqlitedb);
    q.prepare(sql);

    if (!gruppo.isEmpty() && gruppo != "__NO_GROUP__")
        q.bindValue(":gruppo", gruppo);

    if (!anno.isEmpty())
        q.bindValue(":anno", anno);

    if (mese > 0)
        q.bindValue(":mese", QString("%1").arg(mese, 2, 10, QChar('0')));

    // --- Esecuzione query ---
    if (!q.exec()) {
        QMessageBox::warning(this, tr("Errore"), tr("Errore nella cancellazione:\n%1") + q.lastError().text());
        return;
    }

    aggiornaTotaleCassa();

    // Aggiorna vista TreeView e tabella
    populateTreeView(ui->pubTreeview->isChecked());
    tableModel->setQuery("SELECT NULL WHERE 0");  // svuota la tabella
    aggiornaEntrateUscite();

    QMessageBox::information(this, tr("Completato"), tr("Movimenti eliminati."));
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_btnAnnulla_clicked()
{
    editState = EditState::Idle;
    aggiornaPulsanti();
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_btnSalva_clicked()
{
    // -------------------------------------------------
    // 0. Controllo stato
    // -------------------------------------------------
    if (editState != EditState::Creating &&
        editState != EditState::Editing)
        return;

    // -------------------------------------------------
    // 1. Validazione importo
    // -------------------------------------------------
    QString txtImporto = ui->lndImporto->text().trimmed();

    bool ok;
    double importo = txtImporto.toDouble(&ok);

    if (!ok || importo == 0.0)
    {
        QMessageBox::warning(this, tr("Errore"), tr("Inserire un importo valido (es. 1234.56)"));
        ui->lndImporto->setFocus();
        return;
    }

    // -------------------------------------------------
    // 2. Validazione data
    // -------------------------------------------------
    QDate data = ui->dateEdit->date();

    if (!data.isValid())
    {
        QMessageBox::warning(this, tr("Errore"),
                             tr("Data non valida."));
        ui->dateEdit->setFocus();
        return;
    }

    QString dataSql = data.toString("yyyy-MM-dd");

    // -------------------------------------------------
    // 3. Lettura altri campi
    // -------------------------------------------------
    QString gruppo = ui->lblGroup->text().trimmed();
    QString note   = ui->lndNote->text().trimmed();

    if (gruppo.isEmpty())
        gruppo = QString();   // NULL nel DB

    // -------------------------------------------------
    // 4. INSERT oppure UPDATE
    // -------------------------------------------------
    QSqlQuery q(sqlitedb);

    if (editState == EditState::Creating)
    {
        q.prepare(
            "INSERT INTO cassa (data, movimento, gruppo, note) "
            "VALUES (:data, :movimento, :gruppo, :note)"
            );
    }
    else  // EditState::Editing
    {
        if (currentRecordId < 0)
        {
            QMessageBox::warning(this, tr("Errore"), tr("Nessun record selezionato."));
            return;
        }

        q.prepare(
            "UPDATE cassa SET "
            "data = :data, "
            "movimento = :movimento, "
            "gruppo = :gruppo, "
            "note = :note "
            "WHERE id = :id"
            );

        q.bindValue(":id", currentRecordId);
    }

    // -------------------------------------------------
    // 5. Binding comune
    // -------------------------------------------------
    q.bindValue(":data", dataSql);
    q.bindValue(":movimento", importo);
    q.bindValue(":gruppo", gruppo);
    q.bindValue(":note", note);

    if (!q.exec())
    {
        QMessageBox::critical(this, tr("Errore DB"), q.lastError().text());

        return;
    }

    aggiornaTotaleCassa();

    // -------------------------------------------------
    // 6. Reset stato e aggiornamento UI
    // -------------------------------------------------
    EditState oldState = editState;
    editState = EditState::Idle;
    aggiornaPulsanti();

    // -------------------------------------------------
    // 7. Gestione post-salvataggio
    // -------------------------------------------------
    if (oldState == EditState::Creating)
    {
        QMessageBox::StandardButton res =
            QMessageBox::question(
                this,
                tr("Salvato"),
                tr("Movimento salvato correttamente.\n\nVuoi inserirne un altro?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes
                );

        if (res == QMessageBox::Yes)
        {
            // Resta in modalità creazione
            editState = EditState::Creating;
            aggiornaPulsanti();

            // Mantiene la data dell'ultimo inserimento
            ui->dateEdit->setDate(data);

            // Pulisce i campi
            ui->lndImporto->clear();
            ui->lndNote->clear();
            ui->lblGroup->clear();

            ui->lndImporto->setFocus();
            return;
        }
    }

    // Caso modifica per risposta NO esco
    if (oldState == EditState::Editing){
        QMessageBox::information(this, tr("Salvato"), tr("Record modificato salvato."));
    }

    // =============================
    // Aggiornamento TreeView + Tabella
    // =============================
    QString gruppoSel = gruppo.isEmpty() ? "__NO_GROUP__" : gruppo;
    QString annoSel   = data.toString("yyyy");
    int meseSel       = data.month();

    populateTreeView(ui->pubTreeview->isChecked());
    selezionaTreeView(gruppoSel, annoSel, meseSel);
    elaboraPeriodo(gruppoSel, annoSel, meseSel);

}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_btnModifica_clicked()
{
    editState = EditState::Editing;
    aggiornaPulsanti();
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_btnElimina_clicked()
{
    // ------------------------------------
    // 1. Controllo selezione
    // ------------------------------------
    if (currentRecordId < 0)
    {
        QMessageBox::warning(this, tr("Attenzione"), tr("Nessun record selezionato."));
        return;
    }

    // ------------------------------------
    // 2. Conferma utente
    // ------------------------------------
    QString msg = tr("Data: %1\nImporto: %2\n\nVuoi eliminare questo movimento?").arg(ui->dateEdit->text(), ui->lndImporto->text());
    if (QMessageBox::question(
            this,
            tr("Conferma eliminazione"),
            msg,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            ) != QMessageBox::Yes)
    {
        return;
    }
    // ------------------------------------
    // 3. DELETE dal database
    // ------------------------------------
    QSqlQuery q(sqlitedb);
    q.prepare("DELETE FROM cassa WHERE id = :id");
    q.bindValue(":id", currentRecordId);

    if (!q.exec())
    {
        QMessageBox::critical(this,
                              tr("Errore DB"),
                              q.lastError().text());
        return;
    }

    aggiornaTotaleCassa();

    // ------------------------------------
    // 4. Reset stato applicazione
    // ------------------------------------
    currentRecordId = -1;
    editState = EditState::Idle;

    aggiornaPulsanti();

    // ------------------------------------
    // 5. Aggiornamento viste
    // ------------------------------------
    populateTreeView(ui->pubTreeview->isChecked());
    tableModel->setQuery("SELECT NULL WHERE 0");  // svuota tabella
    aggiornaEntrateUscite();

    QMessageBox::information(this,tr("Eliminato"),tr("Movimento eliminato correttamente."));
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_btnStampa_clicked()
{
    if (!tableModel || tableModel->rowCount() == 0)
    {
        QMessageBox::information(this, tr("Stampa"), tr("Nessun dato da stampare."));
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize::A4);
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(15, 15, 15, 15),
                           QPageLayout::Millimeter);

    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Stampa movimenti"));

    if (dialog.exec() != QDialog::Accepted)
        return;

    stampaMovimenti(printer);
}
//*******************************************************************************
//*******************************************************************************
void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dlg(this);
    dlg.exec();
}
//*******************************************************************************
//*******************************************************************************

//========================================================================================
//========FUNZIONI =======================================================================
//========================================================================================
bool MainWindow::inizializzaDatabase()
{
   // QString exePath = QCoreApplication::applicationDirPath();
   // QString dbPath = exePath + "/cassa.db";

   //PATH: ~/.local/share/Brigio/cash-register/cassa.db
   //                  OR
   //$XDG_DATA_HOME/Brigio/cash-register/cassa.db
   QString dataPath = QStandardPaths::writableLocation(
       QStandardPaths::AppDataLocation
       );

   QDir dir;
   if (!dir.exists(dataPath))
       dir.mkpath(dataPath);

   QString dbPath = dataPath + "/cassa.db";

   bool esisteDB = QFile::exists(dbPath);

    sqlitedb = QSqlDatabase::addDatabase("QSQLITE");
    sqlitedb.setDatabaseName(dbPath);

    if (!sqlitedb.open()) {
        QMessageBox::warning(this, tr("Errore DB"), tr("Impossibile aprire il database."));
        return false;
    }

    if (!esisteDB) {
        QSqlQuery query(sqlitedb);
        QString createTable =
            "CREATE TABLE cassa ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "data TEXT NOT NULL, "
            "movimento REAL NOT NULL, "
            "gruppo TEXT, "
            "note TEXT"
            ");";

        if (!query.exec(createTable)) {
            QMessageBox::warning(this, tr("Errore"), tr("Impossibile creare la tabella."));
            return false;
        }
    }

    return true;
}
//========================================================
//========================================================
void MainWindow::populateTreeView(bool showByGroup)
{
    // Icone
    QIcon iconAnno(":/icons/year.png");
    QIcon iconMese(":/icons/month.png");
    QIcon iconGroup(":/icons/group.png");

    // MONTH NAME FUNCTION
    auto nomeMese = [](int mese) -> QString {
        QLocale loc;   // usa la lingua corrente dell'app
        return loc.monthName(mese, QLocale::LongFormat);
    };

    // RESET MODEL
    treeModel = new QStandardItemModel(this);
    treeModel->setHorizontalHeaderLabels(QStringList() << tr("Grouping"));
    ui->trvDate->setModel(treeModel);

    QSqlQuery query(sqlitedb);

    if (!showByGroup)
    {
        // VIEW: YEAR → MONTH
        query.prepare(
            "SELECT DISTINCT strftime('%Y', data) AS Anno, "
            "strftime('%m', data) AS Mese "
            "FROM cassa "
            "ORDER BY Anno, Mese"
            );
    }
    else
    {
        // VIEW: GROUP → YEAR → MONTH
        query.prepare(
            "SELECT DISTINCT IFNULL(gruppo, '') AS Gruppo, "
            "strftime('%Y', data) AS Anno, "
            "strftime('%m', data) AS Mese "
            "FROM cassa "
            "ORDER BY Gruppo, Anno, Mese"
            );
    }

    if (!query.exec()) {
               //=======INTERVIENE========//
      QMessageBox::warning(this, tr("Errore DB"), query.lastError().text());
      return;
    }

    //if (!query.exec()) {
        //qDebug() << "populateTreeView: DB non ancora pronto:"
          //       << query.lastError().text();
        //return;   // silenzioso all'avvio
    //}

    QStandardItem *groupItem = nullptr;
    QStandardItem *yearItem  = nullptr;

    QString lastGroup, lastYear;

    while (query.next())
    {
        if (!showByGroup)
        {
            // -------------------------
            // MODE: YEAR → MONTH
            // -------------------------
            QString year  = query.value(0).toString();
            QString month = query.value(1).toString();

            // NEW YEAR
            if (year != lastYear)
            {
                yearItem = new QStandardItem(iconAnno, year);
                yearItem->setData("year", Qt::UserRole + 1);
                yearItem->setData(year, Qt::UserRole + 2);

                treeModel->appendRow(yearItem);
                lastYear = year;
            }

            // MONTH
            QStandardItem *meseItem = new QStandardItem(iconMese, nomeMese(month.toInt()));
            meseItem->setData("month", Qt::UserRole + 1);
            meseItem->setData(month.toInt(), Qt::UserRole + 2);

            yearItem->appendRow(meseItem);
        }
        else
        {
            // -------------------------
            // MODE: GROUP → YEAR → MONTH
            // -------------------------
            QString group = query.value(0).toString();
            QString year  = query.value(1).toString();
            QString month = query.value(2).toString();

            // NEW GROUP
            if (groupItem == nullptr || group != lastGroup)
            {
                groupItem = new QStandardItem(
                    iconGroup,
                    //group.isEmpty() ? "<Nessun Gruppo>" : group
                    group.isEmpty() ? "" : group
                    );

                groupItem->setData("group", Qt::UserRole + 1);
                //groupItem->setData(group, Qt::UserRole + 2);
                // Salvo un valore speciale per riconoscere il gruppo vuoto
                groupItem->setData(group.isEmpty() ? "__NO_GROUP__" : group, Qt::UserRole + 2);

                treeModel->appendRow(groupItem);

                lastGroup = group;
                lastYear = "";
                yearItem = nullptr;
            }

            // NEW YEAR
            if (yearItem == nullptr || year != lastYear)
            {
                yearItem = new QStandardItem(iconAnno, year);
                yearItem->setData("year", Qt::UserRole + 1);
                yearItem->setData(year, Qt::UserRole + 2);

                groupItem->appendRow(yearItem);

                lastYear = year;
            }

            // MONTH
            QStandardItem *meseItem = new QStandardItem(iconMese, nomeMese(month.toInt()));
            meseItem->setData("month", Qt::UserRole + 1);
            meseItem->setData(month.toInt(), Qt::UserRole + 2);

            yearItem->appendRow(meseItem);
        }
    }
}
//============================================================================
//============================================================================
double MainWindow::calcolaSaldoPrecedente(QString gruppo, QString anno, int mese)
{
    // Se non è stato selezionato un anno (es. selezionato solo il nodo "group"),
    // non esiste un periodo "precedente" definito: saldo precedente = 0
    if (anno.isEmpty()) {
        return 0.0;
    }

    QString sql =
        "SELECT SUM(movimento) FROM cassa WHERE 1=1 ";

    if (gruppo == "__NO_GROUP__") {
        sql += " AND (gruppo IS NULL OR gruppo = '')";
    }
    else if (!gruppo.isEmpty()) {
        sql += " AND gruppo = :gruppo";
    }

    // Tutto ciò che viene prima del periodo selezionato
    if (!anno.isEmpty()) {
        sql += " AND (strftime('%Y', data) < :anno "
               "OR (strftime('%Y', data) = :anno AND strftime('%m', data) < :mese))";
    }

    QSqlQuery q(sqlitedb);
    q.prepare(sql);

    if (!gruppo.isEmpty() && gruppo != "__NO_GROUP__")
        q.bindValue(":gruppo", gruppo);

    if (!anno.isEmpty())
        q.bindValue(":anno", anno);

    if (mese > 0)
        q.bindValue(":mese", QString("%1").arg(mese, 2, 10, QChar('0')));

    q.exec();
    q.next();

    return q.value(0).toDouble();
}
//============================================================================
//============================================================================
void MainWindow::elaboraPeriodo(QString gruppo, QString anno, int mese)
{
    // ------------------------------------------------------------------
    // 1. Calcolo saldo precedente (solo se spunta attiva)
    // ------------------------------------------------------------------
    double saldoPrecedente = 0;

    if (ui->chkMovPrecedenti->isChecked())
    {
        saldoPrecedente = calcolaSaldoPrecedente(gruppo, anno, mese);

        QLocale locale;
        ui->lblMovPrecedenti->setText(
            locale.toString(saldoPrecedente, 'f', 2) + " " + locale.currencySymbol()
            );
    }
    else
    {
        ui->lblMovPrecedenti->setText("");
    }

    // ------------------------------------------------------------------
    // 2. Query principale (saldo = saldoPrec + somma progressiva)
    // ------------------------------------------------------------------
    QString sql =
        "SELECT "
        "id, "
        "strftime('%d/%m/%Y', data) AS data, "
        "movimento, "
        "(:saldoPrec + SUM(movimento) OVER (ORDER BY date(data), id)) AS saldo, "
        "gruppo, "
        "note "
        "FROM cassa "
        "WHERE 1=1";

    if (gruppo == "__NO_GROUP__") {
        sql += " AND (gruppo IS NULL OR gruppo = '')";
    }
    else if (!gruppo.isEmpty()) {
        sql += " AND gruppo = :gruppo";
    }

    if (!anno.isEmpty())
        sql += " AND strftime('%Y', data) = :anno";

    if (mese > 0)
        sql += " AND strftime('%m', data) = :mese";

    sql += " ORDER BY date(data)";

    QSqlQuery q(sqlitedb);
    q.prepare(sql);

    q.bindValue(":saldoPrec", saldoPrecedente);

    if (!gruppo.isEmpty() && gruppo != "__NO_GROUP__")
        q.bindValue(":gruppo", gruppo);

    if (!anno.isEmpty())
        q.bindValue(":anno", anno);

    if (mese > 0)
        q.bindValue(":mese", QString("%1").arg(mese, 2, 10, QChar('0')));

    if (!q.exec()) {
        QMessageBox::warning(this, tr("Errore"), q.lastError().text());
        return;
    }

    tableModel->setQuery(std::move(q));
    aggiornaIntestazioniTabella();

    ui->tabMovements->hideColumn(COL_ID); // nasconde ID

    aggiornaEntrateUscite();
    aggiornaTotaleCassa();

    //Scroll ultima riga
    if (tableModel->rowCount() > 0)
    {
        ui->tabMovements->resizeColumnsToContents();
        ui->tabMovements->scrollToBottom();
    }
}
//==============================================================================
//===============================================================================
void MainWindow::aggiornaEntrateUscite()
{
    double entrate = 0;
    double uscite = 0;

    int rows = tableModel->rowCount();
    for (int r = 0; r < rows; r++)
    {
        double movimento = tableModel->data(tableModel->index(r, COL_IMPORTO)).toDouble();

        if (movimento > 0)
            entrate += movimento;
        else
            uscite += movimento;   // è negativo
    }

    QLocale locale;
    ui->lblEntrate->setText(locale.toString(entrate, 'f', 2) + " " + locale.currencySymbol());
    ui->lblUscite->setText(locale.toString(uscite, 'f', 2) + " " + locale.currencySymbol());

}
//================================================================================
//==================================================================================
void MainWindow::aggiornaPulsanti()
{
    //Si può scrivere anche così
    //QPushButton *bNuovo   = ui->btnNuovo;
    auto bNuovo   = ui->btnNuovo;
    auto bMod     = ui->btnModifica;
    auto bElim    = ui->btnElimina;
    auto bSalva   = ui->btnSalva;
    auto bAnnulla = ui->btnAnnulla;

    switch (editState)
    {
    case EditState::Idle:
        // Nessuna riga selezionata
        bNuovo->setEnabled(true);
        bMod->setEnabled(false);
        bElim->setEnabled(false);
        bSalva->setEnabled(false);
        bAnnulla->setEnabled(false);
        ui->lndImporto->setEnabled(false);
        ui->lndImporto->setText("");
        ui->lblGroup->setEnabled(false);
        ui->lblGroup->setText("");
        ui->lndNote->setEnabled(false);
        ui->lndNote->setText("");
        ui->dateEdit->setEnabled(false);

        break;

    case EditState::Viewing:
        // Una riga è selezionata
        bNuovo->setEnabled(true);
        bMod->setEnabled(true);
        bElim->setEnabled(true);
        bSalva->setEnabled(false);
        bAnnulla->setEnabled(false);
        break;

    case EditState::Editing:
        //Modifica Record selezionato
        bNuovo->setEnabled(false);
        bMod->setEnabled(false);
        bElim->setEnabled(false);
        bSalva->setEnabled(true);
        bAnnulla->setEnabled(true);
        ui->lndImporto->setEnabled(true);
        ui->lblGroup->setEnabled(true);
        ui->lndNote->setEnabled(true);
        ui->dateEdit->setEnabled(true);
        break;

    case EditState::Creating:
        // Sto creando un record
        bNuovo->setEnabled(false);
        bMod->setEnabled(false);
        bElim->setEnabled(false);
        bSalva->setEnabled(true);
        bAnnulla->setEnabled(true);
        ui->lndImporto->setEnabled(true);
        ui->lndImporto->setText("");
        ui->lblGroup->setEnabled(true);
        ui->lblGroup->setText("");
        ui->lndNote->setEnabled(true);
        ui->lndNote->setText("");
        ui->dateEdit->setEnabled(true);
        ui->dateEdit->setDate(QDate::currentDate());
        ui->dateEdit->setDisplayFormat("dd/MM/yyyy");
        break;
    }
}
//=====================================================================
//=====================================================================
void MainWindow::aggiornaTitoloGroupBox()
{
    QString text = ui->lndImporto->text().trimmed();

    if (text.isEmpty()) {
        ui->groupBox->setTitle(tr("Movimenti"));
        ui->groupBox->setStyleSheet(
            "QGroupBox { font: 700 14pt 'Noto Sans'; color: black; }");
        return;
    }

    // stati intermedi consentiti dal validator
    if (text == "-" || text.endsWith('.'))
        return;

    bool ok;
    double valore = text.toDouble(&ok);

    if (!ok || valore == 0.0)
        return;

    if (valore > 0.0) {
        ui->groupBox->setTitle(tr("Entrata"));
        ui->groupBox->setStyleSheet(
            "QGroupBox { font: 700 14pt 'Noto Sans'; color: black; }"
            );
    }
    else {
        ui->groupBox->setTitle(tr("Uscita"));
        ui->groupBox->setStyleSheet(
            "QGroupBox { font: 700 14pt 'Noto Sans'; color: red; }"
            );
    }
}
//=====================================================================
//=====================================================================
void MainWindow::selezionaTreeView(QString gruppo, QString anno, int mese)
{
    QModelIndex foundIndex;

    auto *model = treeModel;

    for (int i = 0; i < model->rowCount(); ++i)
    {
        QStandardItem *groupItem = model->item(i);

        // --- Modalità senza gruppi ---
        if (!ui->pubTreeview->isChecked())
        {
            QStandardItem *yearItem = groupItem;

            if (yearItem->data(Qt::UserRole + 2).toString() == anno)
            {
                for (int m = 0; m < yearItem->rowCount(); ++m)
                {
                    QStandardItem *monthItem = yearItem->child(m);
                    if (monthItem->data(Qt::UserRole + 2).toInt() == mese)
                    {
                        foundIndex = monthItem->index();
                        break;
                    }
                }
            }
        }
        else
        {
            // --- Modalità con gruppi ---
            QString groupValue = groupItem->data(Qt::UserRole + 2).toString();
            if (groupValue != gruppo)
                continue;

            for (int y = 0; y < groupItem->rowCount(); ++y)
            {
                QStandardItem *yearItem = groupItem->child(y);
                if (yearItem->data(Qt::UserRole + 2).toString() != anno)
                    continue;

                for (int m = 0; m < yearItem->rowCount(); ++m)
                {
                    QStandardItem *monthItem = yearItem->child(m);
                    if (monthItem->data(Qt::UserRole + 2).toInt() == mese)
                    {
                        foundIndex = monthItem->index();
                        break;
                    }
                }
            }
        }

        if (foundIndex.isValid())
            break;
    }

    if (!foundIndex.isValid())
        return;

    // Espande TUTTA la gerarchia
    QModelIndex parent = foundIndex.parent();
    while (parent.isValid())
    {
        ui->trvDate->expand(parent);
        parent = parent.parent();
    }

    ui->trvDate->setCurrentIndex(foundIndex);
    ui->trvDate->scrollTo(foundIndex, QAbstractItemView::PositionAtCenter);
}
//=====================================================================
//=====================================================================
void MainWindow::onGroupsChanged()
{
    // Torna in stato neutro
    editState = EditState::Idle;
    aggiornaPulsanti();

    // Svuota tabella (evita dati incoerenti)
    tableModel->setQuery("SELECT NULL WHERE 0");

    // Ricostruisci TreeView
    populateTreeView(ui->pubTreeview->isChecked());

    // Reset selezione TreeView
    ui->trvDate->clearSelection();
    ui->trvDate->setCurrentIndex(QModelIndex());

    // Reset gruppo selezionato
    ui->lblGroup->clear();

    aggiornaEntrateUscite();
}
//=====================================================================
//=====================================================================
void MainWindow::stampaMovimenti(QPrinter &printer)
{
    QLocale locale;   // usa la lingua corrente dell'app

    QPainter painter(&printer);
    if (!painter.isActive())
        return;

    painter.setRenderHint(QPainter::TextAntialiasing);

    // SCALA millimetri → pixel
    const qreal dpi = printer.resolution();
    const qreal scale = dpi / 25.4;
    painter.scale(scale, scale);

    QRectF pageRect = printer.pageLayout().paintRect(QPageLayout::Millimeter);

    // === FONT =====================================================
    QFont fontTitle("Sans Serif");
    fontTitle.setPixelSize(8);
    fontTitle.setBold(true);

    QFont fontHeader("Sans Serif");
    fontHeader.setPixelSize(5);
    fontHeader.setBold(true);

    QFont fontBody("Sans Serif");
    fontBody.setPixelSize(4);

    QFont fontSummary("Sans Serif");
    fontSummary.setPixelSize(5);
    fontSummary.setBold(true);

    qreal left   = pageRect.left();
    qreal top    = pageRect.top();
    qreal bottom = pageRect.bottom();

    qreal x = left;
    qreal y = top;


    // === TITOLO ===================================================
    painter.setFont(fontTitle);
    painter.drawText(
        QRectF(left, y, pageRect.width(), 8),
        Qt::AlignCenter,
        tr("Movimenti")
        );

    y += 12;

    // === COLONNE (mm) =============================================
    QVector<qreal> colWidths { 25, 60, 25, 25, 25 };
    QStringList headers {
        tr("Data"),
        tr("Note"),
        tr("Entrate"),
        tr("Uscite"),
        tr("Saldo")
    };

    qreal tableWidth = 0;
    for (qreal w : colWidths)
        tableWidth += w;

    if (tableWidth < pageRect.width())
        x = left + (pageRect.width() - tableWidth) / 2.0;

    qreal rowHeight = 7;

    // === HEADER ===================================================
    painter.setFont(fontHeader);

    qreal cx = x;
    for (int c = 0; c < headers.size(); ++c)
    {
        painter.drawRect(QRectF(cx, y, colWidths[c], rowHeight));
        painter.drawText(QRectF(cx + 1, y,
                                colWidths[c] - 2, rowHeight),
                         Qt::AlignCenter,
                         headers[c]);
        cx += colWidths[c];
    }

    y += rowHeight;

    // === DATI =====================================================
    painter.setFont(fontBody);

    int rows = tableModel->rowCount();
    double saldoFinale = 0.0;

    for (int r = 0; r < rows; ++r)
    {
        if (y + rowHeight > bottom)
        {
            printer.newPage();
            y = top;
        }

        cx = x;

       // QDate d = tableModel->data(tableModel->index(r, COL_DATA)).toDate();
        //QString data = locale.toString(d, QLocale::ShortFormat);
        QString data = tableModel->data(tableModel->index(r, COL_DATA)).toString();

        double mov    = tableModel->data(tableModel->index(r, COL_IMPORTO)).toDouble();
        double saldo  = tableModel->data(tableModel->index(r, COL_SALDO)).toDouble();
        QString note  = tableModel->data(tableModel->index(r, COL_NOTE)).toString();

        saldoFinale = saldo;

        QString entrata = mov > 0 ? locale.toString(mov, 'f', 2) + " " + locale.currencySymbol() : "";
        QString uscita = mov < 0 ? locale.toString(-mov, 'f', 2) + " " + locale.currencySymbol() : "";

        QStringList row {
            data,
            note,
            entrata,
            uscita,
            locale.toString(saldo, 'f', 2)
        };

        for (int c = 0; c < row.size(); ++c)
        {
            painter.drawRect(QRectF(cx, y, colWidths[c], rowHeight));

            Qt::Alignment align = (c >= 2)
                                      ? Qt::AlignRight | Qt::AlignVCenter
                                      : Qt::AlignLeft  | Qt::AlignVCenter;

            painter.drawText(QRectF(cx + 1, y,
                                    colWidths[c] - 2, rowHeight),
                             align,
                             row[c]);
            cx += colWidths[c];
        }

        y += rowHeight;
    }

    // === RIEPILOGO FINALE ========================================
    y += 6;

    if (y + 20 > bottom)
    {
        printer.newPage();
        y = top;
    }

    painter.setFont(fontSummary);

    // Riga separatrice
    painter.drawLine(QPointF(left, y), QPointF(left + tableWidth, y));
    y += 4;

    // 1) Saldo precedente
    if (ui->chkMovPrecedenti->isChecked())
    {
        painter.drawText(
            QRectF(left, y, tableWidth, rowHeight),
            Qt::AlignLeft | Qt::AlignVCenter,
            tr("Saldo precedente: %1").arg(ui->lblMovPrecedenti->text())
            );
        y += rowHeight;
    }

    // 2) Saldo movimenti
    painter.drawText(
        QRectF(left, y, tableWidth, rowHeight),
        Qt::AlignLeft | Qt::AlignVCenter,
        tr("Saldo del periodo: %1")
            .arg(locale.toString(saldoFinale, 'f', 2) + " " + locale.currencySymbol())
        );
    y += rowHeight;

    // 3) Totale presente in cassa
    painter.drawText(
        QRectF(left, y, tableWidth, rowHeight),
        Qt::AlignLeft | Qt::AlignVCenter,
        tr("Totale Cassa: %1").arg(ui->lblTotale->text())
        );
}
//=====================================================================
//=====================================================================
double MainWindow::calcolaTotaleCassa()
{
    QSqlQuery q(sqlitedb);
    q.prepare("SELECT IFNULL(SUM(movimento), 0) FROM cassa");

    if (!q.exec() || !q.next())
        return 0.0;

    return q.value(0).toDouble();
}
//=====================================================================
//=====================================================================
void MainWindow::aggiornaTotaleCassa()
{
    double totale = calcolaTotaleCassa();

    QLocale locale;
    ui->lblTotale->setText(locale.toString(totale, 'f', 2) + " " + locale.currencySymbol());

}
//=====================================================================
//=====================================================================
void MainWindow::aggiornaTesti()
{

    setWindowTitle(tr("Registro di cassa"));

    // ===== MENU =====
    ui->menuAiuto->setTitle(tr("Aiuto"));

    // ===== AZIONI =====
    ui->actionAbout->setText(tr("Informazioni su…"));

    ui->btnNuovo->setText(tr("Nuovo"));
    ui->btnModifica->setText(tr("Modifica"));
    ui->btnElimina->setText(tr("Elimina"));
    ui->btnSalva->setText(tr("Salva"));
    ui->btnAnnulla->setText(tr("Annulla"));
    ui->btnStampa->setText(tr("Stampa"));

    //Fra tableView e groupBox
    ui->label->setText(tr("Uscite"));
    ui->label_2->setText(tr("Entrate"));
    ui->label_3->setText(tr("Totale Cassa"));

    //Interno GroupBox
    ui->chkMovPrecedenti->setText(tr("Saldo precedente"));
    ui->label_4->setText(tr("Importo"));
    ui->label_6->setText(tr("Gruppo"));
    ui->label_7->setText(tr("Data"));
    ui->label_5->setText(tr("Note"));

    ui->pubTreeview->setText(
        ui->pubTreeview->isChecked()
            ? tr("Vista Date")
            : tr("Vista Gruppo")
        );

    aggiornaTitoloGroupBox(); // Entrata / Uscita
    //aggiornaIntestazioniTabella();
    //I nomi dei mesi si aggiornano automaticamente perché usano QLocale
    //populateTreeView(ui->pubTreeview->isChecked());
}
//=====================================================================
//=====================================================================
void MainWindow::aggiornaIntestazioniTabella()
{
    if (!tableModel)
        return;

    if (tableModel->columnCount() != 6)
        return;

    tableModel->setHeaderData(COL_DATA,     Qt::Horizontal, tr("Data"));
    tableModel->setHeaderData(COL_IMPORTO,  Qt::Horizontal, tr("Importo"));
    tableModel->setHeaderData(COL_SALDO,    Qt::Horizontal, tr("Saldo"));
    tableModel->setHeaderData(COL_GRUPPO,   Qt::Horizontal, tr("Gruppo"));
    tableModel->setHeaderData(COL_NOTE,     Qt::Horizontal, tr("Note"));
}
//=====================================================================
//=====================================================================
void MainWindow::impostaLingua(const QString &codice)
{
    QSettings settings;
    settings.setValue("language", codice);

    qApp->removeTranslator(&translator);

    QString qmFile;
    QLocale locale;

    if (codice == "it") {
        qmFile = ":/translations/Cash_Register_it_IT.qm";
        locale = QLocale(QLocale::Italian, QLocale::Italy);
    }
    else if (codice == "en") {
        qmFile = ":/translations/Cash_Register_en_150.qm";
        locale = QLocale(QLocale::English, QLocale::UnitedKingdom);
    }
    else
        return;

    if (translator.load(qmFile)) {
        qApp->installTranslator(&translator);
        QLocale::setDefault(locale);
    }

    ui->retranslateUi(this);
    aggiornaTesti();
}
//=====================================================================
//=====================================================================
