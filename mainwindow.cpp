#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>

extern "C" {
#include "protocol_parser.h"
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , port(new QSerialPort(this))
    , sendTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Настройка соединений
    connect(sendTimer, &QTimer::timeout, this, &MainWindow::sendNextPacket);
    connect(port, &QSerialPort::readyRead, this, &MainWindow::on_port_ready_read);

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

// Открытие/закрытие порта
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

// Обработка входящих данных
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

// Управление тестированием
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

// Логика тестирования
void MainWindow::startTesting()
{
    currentPacketIndex = 0;
    testPackets = {
        QByteArray::fromHex("AA08000100010000000036"),
        QByteArray::fromHex("AA080001000000000001CA"),
        QByteArray::fromHex("AA0800010400000000F00A")
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

    const QByteArray packet = testPackets[currentPacketIndex++];
    port->write(packet);
    ui->plainTextEdit->appendHtml(QString("<font color='green'>Отправлен пакет %1: %2</font>")
                                 .arg(currentPacketIndex)
                                 .arg(QString::fromUtf8(packet.toHex(' ').toUpper())));
    sendTimer->start(1500);
}

// Вспомогательные методы
void MainWindow::handleParsedPacket()
{
    ui->plainTextEdit->appendHtml("<font color='green'>Пакет разобран</font>");

    if (parser.buffer_length > 4 && parser.buffer[4] == 0x01) {
        ui->plainTextEdit->appendHtml("<font color='red'>Ошибка: 0x01</font>");
        stopTesting();
    }
}

void MainWindow::handleParserError()
{
    ui->plainTextEdit->appendHtml("<font color='red'>Ошибка разбора</font>");
    parser.state = protocol_parser::STATE_SYNC;
}

void MainWindow::stopTesting()
{
    sendTimer->stop();
    testPackets.clear();
    ui->pushButton->setText("Начать");
    ui->plainTextEdit->appendHtml("<font color='orange'>Тестирование остановлено</font>");
}

void MainWindow::on_cleabutt_clicked()
{
    ui->plainTextEdit->clear();
}


