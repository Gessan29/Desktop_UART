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

    auto data = port->readAll();
    ui->plainTextEdit->insertPlainText(data);
}

void MainWindow::on_cleabutt_clicked()
{
    ui->plainTextEdit->clear();
}


void MainWindow::on_pushButton_clicked()
{
     QString text = ui->linebyte->text();
     if (port->isOpen()) {
             port->write(text.toUtf8());
             ui->plainTextEdit->appendPlainText("Отправлено в порт: " + text);
         } else {
             ui->plainTextEdit->appendHtml("<font color='red'>COM порт не открыт!</font> ");
         }

         ui->linebyte->clear();
}

