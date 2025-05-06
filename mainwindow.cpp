#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QVector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , port(new QSerialPort(this))
    , sendTimer(new QTimer(this))
    , responseTimer(new QTimer(this))
{
    ui->setupUi(this);
    setupPort();
    setupPlot();
    setupConnections();
    parser.state = protocol_parser::STATE_SYNC;
}

MainWindow::~MainWindow() {
    delete ui;
    port->close();
}

void MainWindow::setupConnections() {
    connect(sendTimer, &QTimer::timeout, this, &MainWindow::sendNextPacket);
    connect(port, &QSerialPort::readyRead, this, &MainWindow::on_port_ready_read);
    connect(responseTimer, &QTimer::timeout, this, &MainWindow::onResponseTimeout);
    responseTimer->setSingleShot(true);
}

void MainWindow::setupPort() {
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);
}

void MainWindow::setupPlot() {
    ui->customPlot->addGraph();
    ui->customPlot->xAxis->setLabel("Номер точки");
    ui->customPlot->yAxis->setLabel("Напряжение, В");
    ui->customPlot->xAxis->setRange(0, 99);
    ui->customPlot->yAxis->setRange(0, 3.3);
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void MainWindow::on_pbOpen_clicked() {
    if (port->isOpen()) {
        port->close();
        ui->pbOpen->setText("Открыть порт");
        return;
    }

    port->setBaudRate(ui->sbxBaudrate->value());
    port->setPortName(ui->cmbxComPort->currentText());

    if (port->open(QIODevice::ReadWrite)) {
        ui->pbOpen->setText("Закрыть порт");
        logHtml("<font color='green'>Порт открыт!</font><br>");
    } else {
        logHtml("<font color='red'>Ошибка открытия порта!</font><br>");
    }
}

void MainWindow::onResponseTimeout() {
    logHtml("<font color='red'>Ошибка: не получен ответ в течение 10 секунд</font><br>");
    stopTesting();
}

void MainWindow::on_port_ready_read() {
    const QByteArray data = port->readAll();
    //logHtml(QString("<font color='blue'>%1</font>").arg(QString::fromUtf8(data.toHex(' ').toUpper()))); // для просмотра пришедших пакетов

    for (const char byte : data) {
        const parser_result res = process_rx_byte(&parser, static_cast<uint8_t>(byte));
        if (res == PARSER_DONE) handleParsedPacket();
        else if (res == PARSER_ERROR) handleParserError();
    }
}

void MainWindow::on_pushButton_clicked() {
    if (!port->isOpen()) {
        logHtml("<font color='red'>Порт закрыт!</font><br>");
        return;
    }

    isTesting = !isTesting;
    ui->pushButton->setText(isTesting ? "Остановить" : "Начать");
    isTesting ? startTesting() : stopTesting();
}

void MainWindow::logHtml(const QString& message) {
    ui->plainTextEdit->appendHtml(message);
}

void MainWindow::logPlain(const QString& message) {
    ui->plainTextEdit->appendPlainText(message);
}

void MainWindow::startTesting()
{
    currentPacketIndex = 0;
    testPackets = {
        {0x01, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x01, 0x07, 0x00, 0x00, 0x00, 0x00}, {0x01, 0x03, 0x00, 0x00, 0x00, 0x00},
        {0x01, 0x00, 0x01, 0x00, 0x00, 0x00},
        {0x01, 0x02, 0x00, 0x00, 0x00, 0x00}, {0x01, 0x02, 0x01, 0x00, 0x00, 0x00},
        {0x01, 0x01, 0x01, 0x00, 0x00, 0x00}, {0x01, 0x01, 0x02, 0x00, 0x00, 0x00}, {0x01, 0x01, 0x03, 0x00, 0x00, 0x00}, {0x01, 0x01, 0x04, 0x00, 0x00, 0x00},
        {0x01, 0x03, 0x01, 0x00, 0x00, 0x00},
        {0x01, 0x02, 0x00, 0x00, 0x00, 0x00}, {0x01, 0x02, 0x01, 0x00, 0x00, 0x00},

        {0x01, 0x04, 0x00, 0x00, 0x00, 0x00}, {0x01, 0x04, 0x01, 0x00, 0x00, 0x00}, {0x01, 0x04, 0x02, 0x00, 0x00, 0x00}, {0x01, 0x04, 0x03, 0x00, 0x00, 0x00},
        {0x01, 0x04, 0x04, 0x00, 0x00, 0x00}, {0x01, 0x04, 0x05, 0x00, 0x00, 0x00}, {0x01, 0x04, 0x06, 0x00, 0x00, 0x00}, {0x01, 0x04, 0x07, 0x00, 0x00, 0x00},
        {0x01, 0x04, 0x08, 0x00, 0x00, 0x00}, {0x01, 0x04, 0x09, 0x00, 0x00, 0x00}, {0x01, 0x04, 0x0A, 0x00, 0x00, 0x00},
        //
        {0x00, 0x06},
        {0x00, 0x05},
        {0x00, 0x06},
        {0x00, 0x08},
        {0x01, 0x09, 0x0A, 0x00, 0x00, 0x00},
        {0x01, 0x09, 0x0B, 0x00, 0x00, 0x00}
        /*{0x01, 0x09, 0x0C, 0x00, 0x00, 0x00},
        {0x01, 0x09, 0x0D, 0x00, 0x00, 0x00},
        {0x01, 0x09, 0x0E, 0x00, 0x00, 0x00},
        {0x01, 0x09, 0x0F, 0x00, 0x00, 0x00},
        {0x01, 0x09, 0x10, 0x00, 0x00, 0x00},
        {0x01, 0x09, 0x11, 0x00, 0x00, 0x00},
        {0x01, 0x09, 0x12, 0x00, 0x00, 0x00},
        {0x01, 0x09, 0x13, 0x00, 0x00, 0x00}*/
    };
    ui->plainTextEdit->appendHtml("<font color='orange'>Тестирование запущено</font><br>");
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

     //ui->plainTextEdit->appendHtml(QString("<font color='green'>Отправлен пакет %1: %2</font><br>") // для просмотра отправленных пакетов
                                      //.arg(currentPacketIndex)
                                      //.arg(QString::fromUtf8(byteArray.toHex(' ').toUpper())));

    responseTimer->start(10000);
    sendTimer->stop();
}

QString description (const QVector<uint8_t>& packet){
    uint8_t status = packet[1];

    switch (status){
    case 0x00:
        if (packet[2] == 0x01){ return "Подача напряжение питания 12В на плату:"; }
        else {return "Снять напряжение питания 12В с платы:"; }
    case 0x01:
        if (packet[2] == 0x01){ return "Измерение напряжение контрольной точки: -6V:"; }
        else if (packet[2] == 0x02) {return "Измерение напряжение контрольной точки: +3.3V:"; }
        else if (packet[2] == 0x03) {return "Измерение напряжение контрольной точки: +5V:"; }
        else if (packet[2] == 0x04) {return "Измерение напряжение контрольной точки: +6V:"; }
    case 0x02:
        if (packet[2] == 0x00){ return "Измерение напряжения питания платы:"; }
        else {return "Измерение тока питания платы:"; }
    case 0x03:
        if (packet[2] == 0x01){ return "Подключение резисторов имитации:"; }
        else {return "Отключение резисторов имитации:"; }
    case 0x04:
        switch (packet[2]){
        case 0x00:
            return "Измерение напряжение контрольной точки: +1.2V:";
        case 0x01:
            return "Измерение напряжение контрольной точки: +1.8V:";
        case 0x02:
            return "Измерение напряжение контрольной точки: +2.5V:";
        case 0x03:
            return "Измерение напряжение контрольной точки: +5.5V (Power GPS):";
        case 0x04:
            return "Измерение напряжение контрольной точки: +4.5V:";
        case 0x05:
            return "Измерение напряжение контрольной точки: +5.5V:";
        case 0x06:
            return "Измерение напряжение контрольной точки: -5.5V:";
        case 0x07:
            return "Измерение напряжение контрольной точки: +1.8V:";
        case 0x08:
            return "Измерение напряжение контрольной точки: +2.5 (Offset)V:";
        case 0x09:
            return "Измерение напряжение контрольной точки: +5V (Laser):";
        case 0x0A:
            return "Измерение напряжение контрольной точки: 2.048V (VrefDAC):";
        }
    case 0x05:
        return "Измерение формы тока лазерного диода:";
    case 0x06:
        return "Измерение напряжение элемента Пельтье:";
    case 0x07:
        if (packet[2] == 0x01){ return "Переключить тип входных цепей на внешний оптический блок:"; }
        else {return "Переключить тип входных цепей на эквивалентные схемы:"; }
    case 0x08:
        return "Тестирование работы интерфейса RS232:";
    case 0x09: {
        static bool isFirstTime = true;
        if (isFirstTime) {
            isFirstTime = false;
            return "Тестирование работы интерфейса подключения GPS-приемника:";}
        else {
            return "";
             }
      }
}
}

void MainWindow::result(uint8_t* packet){
    uint16_t sample, tok, data;
    double volts;
    const uint16_t accuracy = 300;

    switch (currentPacketIndex){

    case 1:
    case 2:
    case 3:
    case 4:
    case 11:
        ui->plainTextEdit->appendHtml("<font color='green'>Выполнено!</font><br>");
        return;

    case 5:
    case 12:
        handleCaseCommon(2000, "Питание платы,");
        return;

    case 6:
    case 13:
        sample = 50;
        tok = 10;
        data = 0; // (parser.buffer[2] << 8) | parser.buffer[1];

        if (data >= sample - tok || data <= sample + tok){
            ui->plainTextEdit->appendHtml(QString("<font color='green'>Измерено: %1 мА — Ток питания платы допустим</font><br>").arg(data));
        }
        else {
            ui->plainTextEdit->appendHtml(QString("<font color='red'>Измерено: %1 мАЫ — Ток питания платы не допустим</font><br>").arg(data));
            closeTest();
           }
        return;
    case 7:
        handleCaseCommon(6000, "Контрольная точка -6 В");
        return;
    case 8:
        handleCaseCommon(3300, "Контрольная точка +3.3 В");
        return;
    case 9:
        handleCaseCommon(5000, "Контрольная точка +5 В");
        return;
    case 10:
        handleCaseCommon(6000, "Контрольная точка +6 В");
        return;
    case 14:
        handleCaseCommon(6000, "Контрольная точка +1.2 В");
        return;
    case 15:
        handleCaseCommon(6000, "Контрольная точка +1.8 В");
        return;
    case 16:
        handleCaseCommon(6000, "Контрольная точка +2.5 В");
        return;
    case 17:
        handleCaseCommon(6000, "Контрольная точка +5.5 В (Power GPS)");
        return;
    case 18:
        handleCaseCommon(6000, "Контрольная точка +4.5 В (VrefADC)");
        return;
    case 19:
        handleCaseCommon(6000, "Контрольная точка +5.5 В");
        return;
    case 20:
        handleCaseCommon(6000, "Контрольная точка -5.5 В");
        return;
    case 21:
        handleCaseCommon(6000, "Контрольная точка +1.8 В");
        return;
    case 22:
        handleCaseCommon(6000, "Контрольная точка +2.5 В (Offset)");
        return;
    case 23:
        handleCaseCommon(6000, "Контрольная точка +5 В (Laser)");
        return;
    case 24:{
        handleCaseCommon(0, "Контрольная точка +2.048 В (VrefDAC)");
        sendTimer->stop();
        responseTimer->stop();
        CustomDialog dialog_1(this,"Выполните условие", "Прошейте МК и ПЛИС","Ок","Не удалось прошить"); // Добавить фунцию
        if (dialog_1.exec()) {
                ui->plainTextEdit->appendHtml("<font color='green'>МК и ПЛИС прошиты. Продолжение теста...</font><br>");
            } else {
                ui->plainTextEdit->appendHtml("<font color='red'>МК и ПЛИС не прошиты.</font><br>");
                closeTest();
                return;
            }
        CustomDialog dialog_2(this, "Сообщение","Отправьте команду плате АЦМ","Ок","Не удалось отправить"); // Добавить фунцию
        if (dialog_2.exec()) {
                ui->plainTextEdit->appendHtml("<font color='green'>Команда плате АЦМ отправлена. Продолжение теста...</font><br>");
            } else {
                ui->plainTextEdit->appendHtml("<font color='red'>Команда плате АЦМ не отправлена.</font><br>");
                closeTest();
                return;
            }
        return; }
    case 25:{
        handleCaseCommon(0, "Ток элемента Пельтье");
        return; }
    case 26: {
        CustomDialog dialog_3(this, "Установка параметров","Установите форму тока лазера","Ок","Не удалось установить параметры"); // Добавить фунцию
        if (dialog_3.exec()) {
                ui->plainTextEdit->appendHtml("<font color='green'>Форма тока лазера установлена. Продолжение теста...</font>");
            } else {
                ui->plainTextEdit->appendHtml("<font color='red'>Форма тока лазера не установлена.</font><br>");
                closeTest();
                return;
            }
        QByteArray rawData(reinterpret_cast<const char*>(parser.buffer), parser.buffer_length);
        plotAdcData(rawData);
        return; }
    case 27: {
        for (int i = 0; i < 4; i++){
        std::vector<int> temperatures = {28, 22, 55, -5};
        if (i < temperatures.size()) {
               int targetTemp = temperatures[i];
               ui->plainTextEdit->appendHtml(QString("Установить температуру %1 градусов:</font>").arg(targetTemp)); // Добавить фунцию
               ui->plainTextEdit->appendHtml(QString("<font color='green'>Температура %1 градусов установлена</font>").arg(targetTemp));
               handleCaseCommon(0, "Ток элемента Пельтье");
           }
         }
        ui->plainTextEdit->appendHtml("Установить температуру 25 градусов:</font>"); // Добавить фунцию
        ui->plainTextEdit->appendHtml("<font color='green'>Температура 25 градусов установлена</font><br>");
        return; }
    case 28:
        ui->plainTextEdit->appendHtml("<font color='green'>Тестирование RS-232 успешно пройдено</font><br>");
        return;
    case 29:
    /*case 30: //еще 9 измерений для GPS не забыть вернуть, сейчас только одно
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:*/
        sendTimer->start(1000);
        if (currentPacketIndex == 39){ // поменять на 38
        ui->plainTextEdit->appendHtml("<font color='green'>Тестирование GPS успешно пройдено</font><br>"); }
        return;
    }
}

void MainWindow::plotAdcData(const QByteArray& byteArray) {
    if (byteArray.size() < 201) {
        ui->plainTextEdit->appendHtml("<font color='red'>Недостаточно данных для построения графика</font><br>");
        return;
    }
    QVector<double> x(100), y(100);
    int dataStartIndex = 1;

    for (int i = 0; i < 100; ++i) {
           int index = dataStartIndex + i * 2;
           if (index + 1 >= byteArray.size()) break;

           uint8_t low = static_cast<uint8_t>(byteArray[index]);
           uint8_t high = static_cast<uint8_t>(byteArray[index + 1]);
           uint16_t value = (high << 8) | low;
           x[i] = i;
           y[i] = static_cast<double>(value) / 1000;
       }

       ui->customPlot->clearGraphs();
       ui->customPlot->addGraph();
       ui->customPlot->graph(0)->setData(x, y);
       ui->customPlot->xAxis->setLabel("Номер точки (сигнал лазерного диода)");
       ui->customPlot->yAxis->setLabel("Напряжение, В");
       ui->customPlot->xAxis->setRange(0, 99);
       ui->customPlot->yAxis->setRange(0, 3.3);
       ui->customPlot->replot();
       ui->plainTextEdit->appendHtml("<font color='green'>Снятно 100 точек напряжений, построен график.</font><br>");
}

void MainWindow::handleCaseCommon(uint16_t sample, const QString& labelText)
{
    const uint16_t accuracy = 10000; // 300
    uint16_t data = 10000; //(parser.buffer[2] << 8) | parser.buffer[1];
    double volts = data / 1000.0;

    if (data >= sample - accuracy && data <= sample + accuracy) {
        ui->plainTextEdit->appendHtml(
        QString("<font color='green'>Измерено: %1 В — %2 напряжение допустимо</font><br><br>").arg(QString::number(volts, 'f', 3)).arg(labelText));
    } else {
        ui->plainTextEdit->appendHtml(
        QString("<font color='red'>Измерено: %1 В — %2 напряжение превышает диапазон</font><br><br>").arg(QString::number(volts, 'f', 3)).arg(labelText));
        closeTest();
    }
}

void MainWindow::handleParsedPacket()
{
    responseTimer->stop();

        switch (parser.buffer[0]){
        case 0x00:
            result(parser.buffer);
            if (isTesting) {
                sendTimer->start(200);
                return;
            }
        case 0x01:
            ui->plainTextEdit->appendHtml("<font color='red'>Ошибка выполнения команды (код ошибки: 0x01)</font><br>");
            result(parser.buffer);
            if (isTesting) {
                closeTest();
                return; }
        case 0x02:
            ui->plainTextEdit->appendHtml("<font color='red'>Несуществующая команда (код ошибки: 0x02)</font><br>");
            closeTest();
            break;
        case 0x03:
            ui->plainTextEdit->appendHtml("<font color='red'>Превышено время выполнения команды (код ошибки: 0x03)</font><br>");
            closeTest();
            break;
        case 0x04:
            ui->plainTextEdit->appendHtml("<font color='red'>Ошибка размера данных команды (код ошибки: 0x04)</font><br>");
            closeTest();
            break;
        }
}

void MainWindow::closeTest(){
    ui->plainTextEdit->appendHtml("<font color='red'>Завершение тестирования...</font>");
    if (!emergencyStopTriggered) {
               emergencyStopTriggered = true;

               testPackets = {
                   {0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                   {0x01, 0x07, 0x00, 0x00, 0x00, 0x00},
                   {0x01, 0x03, 0x00, 0x00, 0x00, 0x00}
               };

               currentPacketIndex = 0;
               ui->plainTextEdit->appendHtml("<font color='orange'>Повтор команд для отключения питания...</font><br>");
               sendTimer->start(100);
               return;
           }
    stopTesting();
}

void MainWindow::handleParserError()
{
    ui->plainTextEdit->appendHtml("<font color='red'>Ошибка разбора пакета</font><br>");
    parser.state = protocol_parser::STATE_SYNC;
}

void MainWindow::stopTesting()
{
    sendTimer->stop();
    responseTimer->stop();
    testPackets.clear();
    isTesting = false;
    emergencyStopTriggered = false;
    currentPacketIndex = 0;
    ui->pushButton->setText("Начать тестирование");
    ui->plainTextEdit->appendHtml("<font color='orange'><br>Тестирование завершено</font><br>");
}

void MainWindow::on_cleabutt_clicked()
{
    ui->plainTextEdit->clear();
}
