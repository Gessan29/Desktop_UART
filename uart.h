#ifndef UART_H
#define UART_H

#include <QComboBox>

class uart : public QComboBox
{
    Q_OBJECT
public:
    explicit uart(QWidget * parent );

signals:
    void clicked();

protected:
    virtual void showPopup();
    void update_port_list();
};
#endif // UART_H
