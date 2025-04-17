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
    , responseTimer(new QTimer(this)) // Новый таймер ожидания ответа
{
    ui->setupUi(this);

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
                QByteArray::fromHex("AA08000100010000000036"),

                QByteArray::fromHex("AA08000101010000003DF6"),
                QByteArray::fromHex("AA08000101020000003DB2"),
                QByteArray::fromHex("AA08000101030000003C4E"),
                QByteArray::fromHex("AA08000101040000003D3A"),

                QByteArray::fromHex("AA0800010200000000780A"),
                QByteArray::fromHex("AA080001020100000079F6"),

                QByteArray::fromHex("AA08000103010000004436"),

                QByteArray::fromHex("AA0800010400000000F00A"),
                QByteArray::fromHex("AA0800010401000000F1F6"),
                QByteArray::fromHex("AA0800010402000000F1B2"),
                QByteArray::fromHex("AA0800010403000000F04E"),
                QByteArray::fromHex("AA0800010404000000F13A"),
                QByteArray::fromHex("AA0800010405000000F0C6"),
                QByteArray::fromHex("AA0800010406000000F082"),
                QByteArray::fromHex("AA0800010407000000F17E"),
                QByteArray::fromHex("AA0800010408000000F26A"),
                QByteArray::fromHex("AA0800010409000000F396"),
                QByteArray::fromHex("AA080001040A000000F3D2"),

                QByteArray::fromHex("AA0800010700000000B40A"),

                QByteArray::fromHex("AA04000005C1B3"),

                QByteArray::fromHex("AA0400000681B2"),

                //QByteArray::fromHex("AA040000080076"), // RS-232

                //QByteArray::fromHex("AA08000109 0A 000000DE13"),//GPS
                //QByteArray::fromHex("AA08000109 0B 000000DE13"),
                //QByteArray::fromHex("AA08000109 0C 000000DE13"),

                QByteArray::fromHex("AA080001000000000001CA"),

                QByteArray::fromHex("AA0800010701000000B5F6"),
                //???
                QByteArray::fromHex("AA080001030000000045CA")

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

    // Запускаем таймер ожидания ответа, ОСТАНАВЛИВАЕМ отправку следующих пакетов
    responseTimer->start(10000);
    sendTimer->stop();
}

void MainWindow::handleParsedPacket()
{
    responseTimer->stop(); // Ответ получен — остановить таймер


        switch (parser.buffer[0]){
        case 0x00:
            ui->plainTextEdit->appendHtml("<font color='green'>Пакет успешно разобран</font>");
            if (isTesting) {
                sendTimer->start(200); // задержка перед след пакетом
                return;
            }
        case 0x01:
            ui->plainTextEdit->appendHtml("<font color='red'>Ошибка выполнения команды (код ошибки: 0x01)</font>");
            if (isTesting) {
                sendTimer->start(200);
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
    responseTimer->stop(); //  Остановить таймер, если тест завершён
    testPackets.clear();
    isTesting = false;
    ui->pushButton->setText("Начать тестирование");
    ui->plainTextEdit->appendHtml("<font color='orange'>Тестирование завершено</font>");
}

void MainWindow::on_cleabutt_clicked()
{
    ui->plainTextEdit->clear();
}
