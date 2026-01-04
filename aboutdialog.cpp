#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    QPixmap pix(":/icons/app_logo.png");
    ui->lblIco->setPixmap(pix);
    ui->lblIco->setScaledContents(true);

    setWindowTitle(tr("Informazioni su %1").arg(APP_NAME));

    ui->labelAppName->setText(APP_NAME);
    ui->labelVersion->setText(tr("Versione %1").arg(APP_VERSION));

    QString html =
        "<b>%1</b><br>"
        "%2<br><br>"
        "© %3 %4<br>"
        "%5";

    ui->textBrowser->setHtml(
        html.arg(
            tr("Registro di cassa"),
            tr("Applicazione per la gestione del registratore di cassa."),
            "2026",
            tr("Franco Brigiotti"),
            tr("Licenza: GPL v3")
            )
        );
}
//*******************************************************************************
//*******************************************************************************
AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_btnClose_clicked()
{
    accept();
}

