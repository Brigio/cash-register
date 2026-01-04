#ifndef GROUPSWINDOW_H
#define GROUPSWINDOW_H

#include <QDialog>
#include <QSqlDatabase>

namespace Ui {
class GroupsWindow;
}

class GroupsWindow : public QDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent *event) override;

signals:
    void groupsChanged();

public:
    // Modalità di apertura della finestra
    enum class Mode {
        SelectOnly,   // selezione semplice
        FullEdit      // crea / modifica / elimina
    };

    explicit GroupsWindow(QSqlDatabase db,
                          Mode mode,
                          const QString &currentGroup,
                          QWidget *parent = nullptr);

    ~GroupsWindow();

signals:
    // restituisce il gruppo selezionato a MainWindow
    void groupChosen(const QString &group);

private slots:
    void on_lstGroups_itemSelectionChanged();
    void on_btnVai_clicked();
    void on_btnMofifica_clicked();
    void on_btnElimina_clicked();
    void aggiornaTesti();

private:
    Ui::GroupsWindow *ui;
    QSqlDatabase db;
    const QString NO_GROUP_TEXT = tr("<Nessun Gruppo>");
    Mode mode;                 // modalità finestra
    QString currentGroup;      // gruppo proveniente da MainWindow

    void loadGroups();
};

#endif // GROUPSWINDOW_H
