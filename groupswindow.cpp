#include "groupswindow.h"
#include "ui_groupswindow.h"
#include <QSqlQuery>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QSqlError>

GroupsWindow::GroupsWindow(QSqlDatabase dbConn,
                           Mode m,
                           const QString &currentGrp,
                           QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GroupsWindow)
    , db(dbConn)
    , mode(m)
    , currentGroup(currentGrp)

{
    ui->setupUi(this);
    aggiornaTesti();
    loadGroups();


    // ----------------------------
    // Stato iniziale in base alla modalità
    // ----------------------------
    QList<QListWidgetItem*> items =
        ui->lstGroups->findItems(currentGroup, Qt::MatchExactly);
    if (!items.isEmpty())
    {
        ui->lstGroups->setCurrentItem(items.first());
        ui->lneGruppo->setText(currentGroup);
    }
    if (mode == Mode::FullEdit)
    {
        ui->btnVai->setEnabled(true);
    }
    else
    {   
        ui->btnVai->setEnabled(false);
    }

    connect(ui->btnClose, &QPushButton::clicked, this, &GroupsWindow::close);
}

GroupsWindow::~GroupsWindow()
{
    delete ui;
}
//=============================================================================
//==========FUNZIONI=========================================================
//==============================================================================
void GroupsWindow::loadGroups()
{
    ui->lstGroups->clear();

    QSqlQuery q(db);

    q.exec("SELECT DISTINCT IFNULL(gruppo, '') FROM cassa ORDER BY gruppo");

    while (q.next())
    {
        QString g = q.value(0).toString();
        if (g.isEmpty())
            g = NO_GROUP_TEXT;

        ui->lstGroups->addItem(g);
    }
}
//=============================================================================
//==============================================================================
void GroupsWindow::showEvent(QShowEvent *event)
{
    ui->btnVai->setDefault(true);
    QDialog::showEvent(event);
    ui->btnVai->setFocus(Qt::OtherFocusReason);
}
//=============================================================================
//==============================================================================
void GroupsWindow::aggiornaTesti()
{
    setWindowTitle(tr("Gruppi"));

    ui->label->setText(tr("Nome Gruppo:"));

    ui->btnVai->setText(tr("Vai"));
    ui->btnElimina->setText(tr("Elimina"));
    ui->btnClose->setText(tr("Annulla"));
    ui->btnMofifica->setText(tr("Modifica"));
}
//=============================================================================
//==============================================================================

//*******************************************************************************
//*******************************************************************************
void GroupsWindow::on_lstGroups_itemSelectionChanged()
{
    QListWidgetItem *item = ui->lstGroups->currentItem();
    if (!item)
        return;

    QString g = item->text();

    // Gestione gruppo vuoto
    if (g == NO_GROUP_TEXT)
        g.clear();

    ui->lneGruppo->setText(g);
}
//*******************************************************************************
//*******************************************************************************
void GroupsWindow::on_btnVai_clicked()
{
    // Prendo il testo così com'è (anche vuoto)
    QString gruppo = ui->lneGruppo->text().trimmed();

    // Emit del segnale verso MainWindow
    emit groupChosen(gruppo);

    // Chiudo la finestra in modo corretto
    accept();
}
//*******************************************************************************
//*******************************************************************************
void GroupsWindow::on_btnMofifica_clicked()
{
    QListWidgetItem *item = ui->lstGroups->currentItem();
    if (!item)
        return;

    QString oldGroup = item->text();

    // <Nessun Gruppo> → uscita silenziosa
    if (oldGroup == NO_GROUP_TEXT)
        return;

    QString newGroup = ui->lneGruppo->text().trimmed();

    // Nome vuoto non consentito
    if (newGroup.isEmpty())
    {
        QMessageBox::warning(this,
                             tr("Nome non valido"),
                             tr("Il nome del gruppo non può essere vuoto."));
        ui->lneGruppo->setText(oldGroup);
        return;
    }

    // Nessuna modifica reale
    if (newGroup == oldGroup)
        return;

    // Conferma
    QString msg = QString(
                      tr("Vuoi modificare il nome del gruppo?\n\n"
                      "Da: \"%1\"\n"
                      "A:  \"%2\"")
                      ).arg(oldGroup, newGroup);

    if (QMessageBox::question(this,
                              tr("Conferma modifica"),
                              msg,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No)
        != QMessageBox::Yes)
    {
        ui->lneGruppo->setText(oldGroup);
        return;
    }

    // Update DB
    QSqlQuery q(db);
    q.prepare(
        "UPDATE cassa SET gruppo = :newGroup WHERE gruppo = :oldGroup"
        );
    q.bindValue(":newGroup", newGroup);
    q.bindValue(":oldGroup", oldGroup);

    if (!q.exec())
    {
        QMessageBox::critical(this,
                              tr("Errore DB"),
                              q.lastError().text());
        ui->lneGruppo->setText(oldGroup);
        return;
    }

    emit groupsChanged();   // Segnale di sincronizzazione con form registerwindow
    // Aggiorna lista
    loadGroups();

    QList<QListWidgetItem*> items =
        ui->lstGroups->findItems(newGroup, Qt::MatchExactly);

    if (!items.isEmpty())
        ui->lstGroups->setCurrentItem(items.first());

    ui->lneGruppo->setText(newGroup);
}
//*******************************************************************************
//*******************************************************************************
void GroupsWindow::on_btnElimina_clicked()
{
    QListWidgetItem *item = ui->lstGroups->currentItem();
    if (!item)
        return;

    QString selectedGroup = item->text();

    // <Nessun Gruppo> → uscita silenziosa
    if (selectedGroup == NO_GROUP_TEXT)
        return;

    QString editGroup = ui->lneGruppo->text().trimmed();

    if (editGroup != selectedGroup)
    {
        QMessageBox::warning(this,
                             tr("Nome modificato"),
                             tr("Per eliminare un gruppo non modificarne il nome."));
        ui->lneGruppo->setText(selectedGroup);
        return;
    }

    QString msg = QString(
                      tr("Vuoi eliminare il gruppo \"%1\"?\n\n"
                         "I movimenti associati verranno impostati senza gruppo.")
                      ).arg(selectedGroup);

    if (QMessageBox::question(this,
                              tr("Conferma eliminazione"),
                              msg,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No)
        != QMessageBox::Yes)
        return;

    QSqlQuery q(db);
    q.prepare(
        "UPDATE cassa SET gruppo = NULL WHERE gruppo = :grp"
        );
    q.bindValue(":grp", selectedGroup);

    if (!q.exec())
    {
        QMessageBox::critical(this,
                              tr("Errore DB"),
                              q.lastError().text());
        return;
    }

    emit groupsChanged();   // Segnale di sincronizzazione con form registerwindow
    loadGroups();
    ui->lneGruppo->clear();
}

