#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtBluetooth/QtBluetooth>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothSocket>
#include <QAction>
#include <fftw3.h>
#include <lo/lo.h>
#include "configdialog.h"
#include "qcustomplot.h"
#include "dendron.h"

#define CHART_DATA_SIZE 1024
#define NUMBER_OF_CHANNELS 6
#define GRAPH_UPDATE_DIVISION 4

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_searchButton_clicked();
    void on_connectButton_clicked();
    void on_bt_device_discovered(const QBluetoothDeviceInfo &device);
    void on_searching_timer_update();
    void on_bt_connected();
    void on_bt_socket_error(QBluetoothSocket::SocketError error);
    void on_bt_socket_data();
    void on_dendron_data();
    void on_startButton_clicked();
    void on_action_Quit_triggered();
    void on_actionA_bout_triggered();
    void on_recordButton_clicked();
    void on_actionConfiguration_triggered();

    void on_markButton_clicked();

    void on_OSCButton_clicked();

private:
    QSettings *config;
    QString saveDataPath;
    QString OSCPath;
    qint32 OSCPort;
    void parse_dendron_data(QByteArray bytes, dendron_data_t *data);
    Ui::MainWindow *ui;
    QBluetoothDeviceDiscoveryAgent *bt_agent;
    QBluetoothSocket *bt_socket;
    bool bt_searching;
    bool bt_connected;
    bool dendron_streaming;
    bool osc_streaming;
    QTimer *searching_timer;
    bool searching_anim;
    dendron_data_t *last_data;
    QCustomPlot *mainPlot;
    int graph_update_counter;
    QVector<double> *channel_x;
    QVector<double> channels_y[NUMBER_OF_CHANNELS];
    int plot_position;
    QFile *record_file;
    QTextStream *file_stream;
    bool recording;
    fftw_complex *in, *out;
    lo_address osc_addr;
};
#endif // MAINWINDOW_H
