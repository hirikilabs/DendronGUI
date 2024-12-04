#include "configdialog.h"
#include "ui_configdialog.h"

ConfigDialog::ConfigDialog(QWidget *parent, QSettings *conf)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
{
    config = conf;
    ui->setupUi(this);
    if (config != nullptr) {
        ui->pathLineEdit->setText(config->value("data/path").toString());
        ui->OSCPathLineEdit->setText(config->value("osc/path").toString());
        ui->OSCPortSpinBox->setValue(config->value("osc/port").toInt());
    }
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::on_buttonBox_accepted()
{
    if (ui->pathLineEdit->text() != "") {
        if (config != nullptr) {
            config->setValue("data/path", ui->pathLineEdit->text());
            config->setValue("osc/path", ui->OSCPathLineEdit->text());
            config->setValue("osc/port", ui->OSCPortSpinBox->value());
        }
    }
    close();
}

void ConfigDialog::on_buttonBox_rejected()
{
    close();
}


void ConfigDialog::on_selectPathButton_clicked()
{
    QFileDialog folderDialog = QFileDialog(this);
    QString folder = folderDialog.getExistingDirectory();
    ui->pathLineEdit->setText(folder);
}

