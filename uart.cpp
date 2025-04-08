#include "uart.h"
#include <QSerialPortInfo>
#include <QDebug>

uart::uart(QWidget * parent)
    : QComboBox(parent)
{}

void uart::showPopup()
{
    auto lastText = currentText();
    update_port_list();
    setCurrentText(lastText);
    QComboBox::showPopup();
}

void uart::update_port_list()
{
    auto portInfoList = QSerialPortInfo::availablePorts();
    clear();
    for(auto & portInfo : portInfoList)
        if(!portInfo.isBusy())
            addItem(portInfo.portName());
}

