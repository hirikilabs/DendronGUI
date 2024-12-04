#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QSettings>

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr, QSettings *conf = nullptr);
    ~ConfigDialog();

private slots:

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void on_selectPathButton_clicked();

private:
    Ui::ConfigDialog *ui;
    QSettings *config;
};

#endif // CONFIGDIALOG_H
