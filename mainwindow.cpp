#include "mainwindow.h"
#include "./ui_mainwindow.h"

QColor plotColors[8] = {QColorConstants::Svg::dodgerblue, QColorConstants::Svg::darkorchid,
                        QColorConstants::Svg::darkseagreen, QColorConstants::Svg::goldenrod,
                        QColorConstants::Svg::fuchsia, QColorConstants::Svg::springgreen,
                        QColorConstants::Svg::darkred, QColorConstants::Svg::slategrey};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    // config
    config = new QSettings("DendronGUI", "MedialabTBK");
    saveDataPath = config->value("data/path").toString();
    OSCPath = config->value("osc/path").toString();
    OSCPort = config->value("osc/port").toInt();
    if (saveDataPath == "") {
        // default values
        saveDataPath = QDir::homePath();
        config->setValue("data/path", saveDataPath);
        OSCPath = "/dendron";
        OSCPort = 5000;
        config->setValue("osc->port", OSCPort);
        config->setValue("osc/path", OSCPath);
    }

    // create BT discovery agent and connect discovered signal to handler
    bt_searching = false;
    bt_agent = new QBluetoothDeviceDiscoveryAgent;
    connect(bt_agent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(on_bt_device_discovered(QBluetoothDeviceInfo)));

    // timer for search button animation
    searching_timer = new QTimer(this);
    connect(searching_timer, SIGNAL(timeout()), this, SLOT(on_searching_timer_update()));

    // not connected
    bt_connected = false;
    bt_socket = nullptr;
    // not receiving data
    dendron_streaming = false;

    // last data received
    last_data = new dendron_data_t;

    // not recording
    recording = false;

    // not sending OSC data
    osc_streaming = false;
    osc_addr = lo_address_new(NULL, QString::number(OSCPort).toLocal8Bit().constData());

    // init plot data, resize vectors to plot X size, and add initial data (x: 0->1023, y: 0)
    graph_update_counter = 0;
    plot_position = 0;
    channel_x = new QVector<double>(CHART_DATA_SIZE);
    for (int i=0; i<NUMBER_OF_CHANNELS; i++) {
        channels_y[i].resize(CHART_DATA_SIZE);
    }

    for (int i=0; i<CHART_DATA_SIZE; ++i)
    {
        channel_x->replace(i, i);
    }
    for (int i=0; i<NUMBER_OF_CHANNELS; i++) {
        for (int j=0; j<CHART_DATA_SIZE; j++) {
            channels_y[i].replace(j, 0);
        }
    }

    // create CustomPlot
    mainPlot = new QCustomPlot();
    for (int i=0; i<NUMBER_OF_CHANNELS; i++) {
        mainPlot->addGraph();
        mainPlot->graph(i)->setData(*channel_x, channels_y[i]);
        mainPlot->graph(i)->setPen(QPen(plotColors[i]));
    }
    // set ranges and style
    mainPlot->xAxis->setRange(0, CHART_DATA_SIZE-1);
    mainPlot->yAxis->setRange(-(1<<24), 1<<24); // 24 bits
    mainPlot->xAxis->setTickLabels(false);
    mainPlot->yAxis->setTickLabels(false);
    mainPlot->replot();

    // set UI
    ui->setupUi(this);
    ui->verticalLayout->insertWidget(1, mainPlot);
    ui->markButton->setStyleSheet("background-color: lightyellow");
}

MainWindow::~MainWindow()
{
    delete config;

    // recording?
    if (recording) {
        record_file->close();
    }

    // disconnect ?
    if (bt_connected) {
        // streaming? stop
        if (dendron_streaming) {
            bt_socket->write("s", 1);
        }
        bt_socket->disconnectFromService();
    }

    // socket created?
    if (bt_socket != nullptr) {
        delete bt_socket;
    }

    delete bt_agent;
    delete ui;
}

void MainWindow::on_searchButton_clicked()
{
    if (!bt_searching) {
        bt_agent->start();
        ui->searchButton->setText("Detener");
        bt_searching = true;
        searching_anim = false;
        searching_timer->start(500);
    } else {
        bt_agent->stop();
        ui->searchButton->setText("Buscar");
        ui->searchButton->setStyleSheet("");
        bt_searching = false;
        searching_timer->stop();
    }
}

void MainWindow::on_searching_timer_update()
{
    if (searching_anim) {
        ui->searchButton->setStyleSheet("background-color: red;");
    } else {
        ui->searchButton->setStyleSheet("");
    }
    searching_anim = !searching_anim;
}

void MainWindow::on_connectButton_clicked()
{
    // try to connect if not connected
    if (!bt_connected) {
        // get selected device
        QString address = ui->devicesComboBox->currentText();

        // RFComm service UUID
        static const QString dendronServiceUuid(QStringLiteral("00001101-0000-1000-8000-00805F9B34FB"));

        bt_socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);
        // if we connect, signal on_bt_connected will be called
        connect(bt_socket, SIGNAL(connected()), this, SLOT(on_bt_connected()));
        // and if there is an error, handle it
        connect(bt_socket, SIGNAL(errorOccurred(QBluetoothSocket::SocketError)),
                this, SLOT(on_bt_socket_error(QBluetoothSocket::SocketError)));
        bt_socket->connectToService(QBluetoothAddress(address), QBluetoothUuid(dendronServiceUuid), QIODevice::ReadWrite);
    } else {
        // disconnect
        bt_socket->disconnectFromService();
        bt_connected = false;
        ui->connectButton->setText("Connect");
        ui->connectButton->setIcon(QIcon(":/icons/connect"));
        ui->statusbar->showMessage("🟥 Disconnected", 0);
        ui->startButton->setEnabled(false);
        ui->searchButton->setEnabled(true);
    }

}

void MainWindow::on_bt_device_discovered(const QBluetoothDeviceInfo &device)
{
    // if the device found is a Dendron device, and the combo does not already have it
    if (device.name().contains("DENDRON")) {
        if (ui->devicesComboBox->findText(device.address().toString()) == -1) {
            ui->devicesComboBox->addItem(device.address().toString());
            ui->connectButton->setEnabled(true);
        }
    }
}

void MainWindow::on_bt_connected() {

    // if BT searching is running, stop it
    if (bt_searching) {
        bt_agent->stop();
        ui->searchButton->setText("Buscar");
        ui->searchButton->setStyleSheet("");
        bt_searching = false;
        searching_timer->stop();
    }

    // ok, show connection status
    ui->statusbar->showMessage("🟢 Connected to: " + bt_socket->peerName() + "\n", 0);
    bt_connected = true;
    ui->connectButton->setText("Disconnect");
    ui->connectButton->setIcon(QIcon(":/icons/disconnect"));
    ui->startButton->setEnabled(true);
    ui->searchButton->setEnabled(false);

    // connect readyRead signal and send command
    connect(bt_socket, SIGNAL(readyRead()), this, SLOT(on_bt_socket_data()));
    // test signal on channel 1
    //bt_socket->write("t1", 2);
}

void MainWindow::on_bt_socket_error(QBluetoothSocket::SocketError error)
{
    qDebug() << "Socket error: " << error;
    QMessageBox::about(this, "Error", "<b>Problem connecting to bluetooth</b>");
}

void MainWindow::on_bt_socket_data() {
    if (bt_socket->bytesAvailable() >= DENDRON_PACKET_SIZE) {
        QByteArray read_data = bt_socket->read(DENDRON_PACKET_SIZE);
        parse_dendron_data(read_data, last_data);
        emit on_dendron_data();
    }
}

void MainWindow::parse_dendron_data(QByteArray bytes, dendron_data_t *data) {
    // packet number
    // data->packet_num = bytes[0] + (bytes[1] << 8) + (bytes[2] << 16) + (bytes[3] << 24);
    data->packet_num = qFromLittleEndian<uint32_t>(bytes.sliced(0, 4));
    // status
    data->status = qFromLittleEndian<uint32_t>(bytes.sliced(4, 8));

    // data
    for (int i=0; i<8; i++) {
        //data->data[i] = bytes[8+(i*4)] + (bytes[9+(i*4)] << 8) + (bytes[10+(i*4)] << 16) + (bytes[11+(i*4)] << 24);
        data->data[i] = qFromLittleEndian<uint32_t>(bytes.sliced(8+i*4, 4));
        if ((data->data[i] & 0x800000) != 0) {
            data->data[i] |= -16777216;
        }
    }

    // battery
    qint32 batt = bytes[44] + (bytes[45] << 8) + (bytes[46] << 16) + (bytes[47] << 24);
    data->battery = double(batt) / 100.0;
}

void MainWindow::on_dendron_data() {
    // qDebug() << "batt: " << last_data->battery;
    // ui->debugTextEdit->append(QString::number(last_data->packet_num) +  " -> " + QString::number(last_data->data[0]));

    for (int i=0; i<NUMBER_OF_CHANNELS; i++) {
        channels_y[i].replace(plot_position, last_data->data[i]);
        mainPlot->graph(i)->setData(*channel_x, channels_y[i]);
    }
    if (graph_update_counter == 0) {
        mainPlot->replot();
    }
    graph_update_counter++;
    if (graph_update_counter >= GRAPH_UPDATE_DIVISION) {
        graph_update_counter = 0;
    }
    plot_position++;
    if (plot_position >= CHART_DATA_SIZE) {
        plot_position = 0;
    }

    // record?
    if (recording) {
        QString csv_line = QString("%1,%2,%3,%4,%5,%6,%7,%8,%9\n").arg(last_data->packet_num)
                       .arg(last_data->data[0])
                       .arg(last_data->data[1])
                       .arg(last_data->data[2])
                       .arg(last_data->data[3])
                       .arg(last_data->data[4])
                       .arg(last_data->data[5])
                       .arg(last_data->data[6])
                       .arg(last_data->data[7]);
        record_file->write(csv_line.toUtf8());
    }

    // OSC?
    if (osc_streaming) {
        lo_send(osc_addr, OSCPath.toLocal8Bit().data(), "iiiiiiii", last_data->data[0],
                last_data->data[1],
                last_data->data[2],
                last_data->data[3],
                last_data->data[4],
                last_data->data[5],
                last_data->data[6],
                last_data->data[7]);
    }
}

void MainWindow::on_startButton_clicked()
{
    if (bt_connected && !dendron_streaming) {
        bt_socket->write("l", 1);
        dendron_streaming = true;
        ui->startButton->setText("Stop");
        ui->startButton->setIcon(QIcon(":/icons/stop"));
        ui->startButton->setStyleSheet("background-color: green;");
        ui->recordButton->setEnabled(true);
        ui->markButton->setEnabled(true);
    } else if (bt_connected && dendron_streaming) {
        bt_socket->write("s", 1);
        dendron_streaming = false;
        ui->startButton->setText("Start");
        ui->startButton->setIcon(QIcon(":/icons/start"));
        ui->startButton->setStyleSheet("");
        ui->recordButton->setEnabled(false);
        ui->markButton->setEnabled(false);
    }
}

void MainWindow::on_action_Quit_triggered()
{
    close();
}


void MainWindow::on_actionA_bout_triggered()
{
    QMessageBox::about(this, "About...", "<b>Dendron GUI</b><br/>Neuro Hacking Open Group<br/>Medialab Tabakalera");
}


void MainWindow::on_recordButton_clicked()
{
    if (!recording) {
        // create file with timestamp
        QDateTime current = QDateTime::currentDateTime();
        QString filename = QString("%1_%2_%3-%4_%5_%6-%7.csv").arg(current.date().year())
                               .arg(current.date().month())
                               .arg(current.date().day())
                               .arg( current.time().hour())
                               .arg(current.time().minute())
                               .arg(current.time().second())
                               .arg(current.time().msec());
        record_file = new QFile(config->value("data/path").toString() + QDir::separator() + filename);
        if(!record_file->open(QFile::WriteOnly |
                       QFile::Text))
        {
            qDebug() << "Could not open file for writing";
            return;
        }
        // set record
        ui->recordButton->setText("Recording");
        ui->recordButton->setStyleSheet("background-color: red;");
        recording = true;
    } else {
        recording = false;
        // close file
        record_file->close();
        ui->recordButton->setText("Record");
        ui->recordButton->setStyleSheet("");
    }
}


void MainWindow::on_actionConfiguration_triggered()
{
    // launch config dialog as modal
    ConfigDialog *confDialog = new ConfigDialog(this, config);
    confDialog->exec();
}


void MainWindow::on_markButton_clicked()
{
    // send mark command if streaming
    if (dendron_streaming) {
        bt_socket->write("m", 1);
    }
}


void MainWindow::on_OSCButton_clicked()
{
    osc_streaming = !osc_streaming;
    if (osc_streaming) {
        ui->OSCButton->setStyleSheet("background-color: yellow;");
    } else {
        ui->OSCButton->setStyleSheet("");
    }
}

