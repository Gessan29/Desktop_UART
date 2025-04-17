#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVector>

extern "C" {
#include "protocol_parser.h"
}

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

//QList<QByteArray> testPackets;

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
    QTimer* responseTimer;
    struct protocol_parser parser;
    QList<QByteArray> testPackets;
    int currentPacketIndex = 0;
    bool isTesting = false;
    QTimer* sendTimer;
};
#endif // MAINWINDOW_H
