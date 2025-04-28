#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVector>
#include "protocol_parser.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

QString description (const QVector<uint8_t>& packet);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_port_ready_read();
    void on_pbOpen_clicked();
    void sendNextPacket();
    void on_cleabutt_clicked();
    void stopTesting();
    void on_pushButton_clicked();
    void handleParserError();
    void handleParsedPacket();
    void startTesting();


private:
    Ui::MainWindow *ui;
    QSerialPort *port;
    QTimer* sendTimer; // таймер для отправки следующих пакетов через задержку
    QTimer* responseTimer; // таймер ожидания ответа от МК
    struct protocol_parser parser;
    QVector<QVector<uint8_t>> testPackets;
    int currentPacketIndex = 0; //индекс текущего пакета в очереди
    bool isTesting = false; // флаг, идет ли сейчас тестирование
    void sendPacket(uint8_t cmd, uint8_t status, uint8_t value);
};
#endif // MAINWINDOW_H
