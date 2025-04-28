#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QVector>

//extern "C" {
//#include "protocol_parser.h"
//}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , port(new QSerialPort(this))
    , sendTimer(new QTimer(this))
    , responseTimer(new QTimer(this)) // Новый таймер ожидания ответа
{
    ui->setupUi(this);
    parser.state = protocol_parser::STATE_SYNC;
    // Настройка соединений
    connect(sendTimer, &QTimer::timeout, this, &MainWindow::sendNextPacket);
    connect(port, &QSerialPort::readyRead, this, &MainWindow::on_port_ready_read);

    // Настройка таймера ожидания ответа
    responseTimer->setSingleShot(true);
    connect(responseTimer, &QTimer::timeout, this, [=]() {
        ui->plainTextEdit->appendHtml("<font color='red'>Ошибка: не получен ответ в течение 10 секунд</font>");
        stopTesting();
    });

    // Настройка параметров порта
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);
}

MainWindow::~MainWindow()
{
    delete ui;
    port->close();
}

void MainWindow::on_pbOpen_clicked()
{
    if (port->isOpen()) {
        port->close();
        ui->pbOpen->setText("Открыть порт");
        return;
    }

    port->setBaudRate(ui->sbxBaudrate->value());
    port->setPortName(ui->cmbxComPort->currentText());

    if (port->open(QIODevice::ReadWrite)) {
        ui->pbOpen->setText("Закрыть порт");
        ui->plainTextEdit->appendHtml("<font color='green'>Порт открыт!</font>");
    } else {
        ui->plainTextEdit->appendHtml("<font color='red'>Ошибка открытия порта!</font>");
    }
}

void MainWindow::on_port_ready_read()
{
    const QByteArray data = port->readAll();
    ui->plainTextEdit->appendHtml(QString("<font color='blue'>%1</font>").arg(QString::fromUtf8(data.toHex(' ').toUpper())));

    for (const char byte : data) {
        const parser_result res = process_rx_byte(&parser, static_cast<uint8_t>(byte));

        if (res == PARSER_DONE) handleParsedPacket();
        else if (res == PARSER_ERROR) handleParserError();
    }
}

void MainWindow::on_pushButton_clicked()
{
    if (!port->isOpen()) {
        ui->plainTextEdit->appendHtml("<font color='red'>Порт закрыт!</font>");
        return;
    }

    isTesting = !isTesting;
    ui->pushButton->setText(isTesting ? "Остановить" : "Начать");

    if (isTesting) startTesting();
    else stopTesting();
}

void MainWindow::startTesting()
{
    currentPacketIndex = 0;
    testPackets = {
        {0x01, 0x00, 0x01, 0x00, 0x00, 0x00},
        {0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x05}
    };
    ui->plainTextEdit->appendHtml("<font color='green'>Тестирование запущено</font>");
    sendNextPacket();
}

void MainWindow::sendNextPacket()
{
    if (!isTesting || currentPacketIndex >= testPackets.size()) {
        stopTesting();
        return;
    }
     const QVector<uint8_t>& packetData = testPackets[currentPacketIndex++];
     ui->plainTextEdit->appendPlainText(description(packetData));
     transfer packet(packetData);
     serialize_reply(&packet);
     int packetSize = 0;
         if (packet.getCmd() == 0) {
             packetSize = 7;
         } else {
             packetSize = 11;
         }

         QByteArray byteArray(reinterpret_cast<const char*>(packet.buf), packetSize);
     port->write(byteArray);

     ui->plainTextEdit->appendHtml(QString("<font color='green'>Отправлен пакет %1: %2</font>")
                                      .arg(currentPacketIndex)
                                      .arg(QString::fromUtf8(byteArray.toHex(' ').toUpper())));

    responseTimer->start(5000);
    sendTimer->stop();
}

QString description (const QVector<uint8_t>& packet){
    uint8_t status = packet [1];

    switch (status){
    case 0x00:
        if (packet [2] == 1){ return "Подача напряжения питания 12В на плату газоанализтора"; }
        else {return "Снять подачу напряжения питания 12В на плату газоанализтора"; }
    case 0x05:
        return "Измерение формы тока лазерного диода";
}
}
void MainWindow::handleParsedPacket()
{
    responseTimer->stop();


        switch (parser.buffer[0]){
        case 0x00:
            ui->plainTextEdit->appendHtml("<font color='green'>Пакет успешно разобран</font>");
            if (isTesting) {
                sendTimer->start(300);
                return;
            }
        case 0x01:
            ui->plainTextEdit->appendHtml("<font color='red'>Ошибка выполнения команды (код ошибки: 0x01)</font>");
            if (isTesting) {
                sendTimer->start(300);
                return; }
        case 0x02:
            ui->plainTextEdit->appendHtml("<font color='red'>Несуществующая команда (код ошибки: 0x02)</font>");
            break;
        case 0x03:
            ui->plainTextEdit->appendHtml("<font color='red'>Превышено время выполнения команды (код ошибки: 0x03)</font>");
            break;
        case 0x04:
            ui->plainTextEdit->appendHtml("<font color='red'>Ошибка размера данных команды (код ошибки: 0x04)</font>");
            break;
        }
        stopTesting();

}

void MainWindow::handleParserError()
{
    ui->plainTextEdit->appendHtml("<font color='red'>Ошибка разбора пакета</font>");
    parser.state = protocol_parser::STATE_SYNC;
}

void MainWindow::stopTesting()
{
    sendTimer->stop();
    responseTimer->stop();
    testPackets.clear();
    isTesting = false;
    ui->pushButton->setText("Начать тестирование");
    ui->plainTextEdit->appendHtml("<font color='orange'>Тестирование завершено</font>");
}

void MainWindow::on_cleabutt_clicked()
{
    ui->plainTextEdit->clear();
}
