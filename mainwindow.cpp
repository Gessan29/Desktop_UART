#include "mainwindow.h"
#include "qdebug.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QClipboard>
#include <QTimer>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , port(new QSerialPort(this))

{

    ui->setupUi(this);
    //connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::on_pushButton_clicked);
    connect(port, &QSerialPort::readyRead, this, &MainWindow::on_port_ready_read);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);
}

MainWindow::~MainWindow()
{
    delete ui;
    port->close();
    delete port;
}

void MainWindow::on_pbOpen_clicked()
{
    if(port->isOpen()) {
        port->close();
        ui->pbOpen->setText(tr("Открыть порт"));
    } else {
        port->setBaudRate(ui->sbxBaudrate->value());
        port->setPortName(ui->cmbxComPort->currentText());
        if(!port->open(QIODevice::ReadWrite)) {
            ui->plainTextEdit->appendPlainText(tr("невозможно открыть порт ") + ui->cmbxComPort->currentText());
        } else {
            ui->pbOpen->setText(tr("Закрыть порт"));
            ui->plainTextEdit->appendHtml("<font color='green'>порт успешно открыт!</font> ");
        }
    }
}

void MainWindow::on_port_ready_read()
{

    QByteArray data = port->readAll();

        // Преобразуем данные в HEX с пробелами и добавим в текстовое поле
        QString hexData = data.toHex(' ').toUpper();
        ui->plainTextEdit->appendHtml(QString("<font color='blue'>Принято: %1</font>").arg(hexData));
}

void MainWindow::on_cleabutt_clicked()
{
    ui->plainTextEdit->clear();
}


void MainWindow::on_pushButton_clicked()
{
    QString text = ui->linebyte->text().trimmed().toUpper().replace(" ", ""); // Удаляем пробелы и лишние символы

       // Проверка на валидность HEX-строки
       if (text.isEmpty() || text.length() % 2 != 0 || !text.contains(QRegularExpression("^[0-9A-F]+$"))) {
           ui->plainTextEdit->appendHtml("<font color='red'>Ошибка: неверный формат HEX (пример: AA0800...)</font><br>");
           ui->linebyte->clear();
           return;
       }

       // Преобразование строки в байты
       QByteArray data = QByteArray::fromHex(text.toLatin1());

       if (port->isOpen()) {
           port->write(data);
           ui->plainTextEdit->appendHtml(
               QString("<br><font color='green'>Отправлено: %1</font><br>")
               .arg(QString(data.toHex(' ').toUpper())) // Форматирование с пробелами
           );
       } else {
           ui->plainTextEdit->appendHtml("<font color='red'>COM порт не открыт!</font><br>");
       }

       ui->linebyte->clear();
}

