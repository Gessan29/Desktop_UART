// файл класса для создания кастомных диалоговых окон
#ifndef CUSTOMDIALOG_H
#define CUSTOMDIALOG_H

#include <QMessageBox>
#include <QPushButton>
#include <QAbstractButton>
#include <QWidget>

class CustomDialog {
public:
    CustomDialog(QWidget *parent,
                 const QString &title,
                 const QString &message,
                 const QString &acceptText,
                 const QString &rejectText)
        : parentWidget(parent),
          dialogTitle(title),
          dialogMessage(message),
          acceptButtonText(acceptText),
          rejectButtonText(rejectText) {}

    bool exec() {
        QMessageBox msgBox(parentWidget);
        msgBox.setWindowTitle(dialogTitle);
        msgBox.setText(dialogMessage);
        QAbstractButton *btnAccept = msgBox.addButton(acceptButtonText, QMessageBox::AcceptRole);
        QAbstractButton *btnReject = msgBox.addButton(rejectButtonText, QMessageBox::RejectRole);
        msgBox.exec();
        return msgBox.clickedButton() == btnAccept;
    }

private:
    QWidget *parentWidget;
    QString dialogTitle;
    QString dialogMessage;
    QString acceptButtonText;
    QString rejectButtonText;
};

#endif // CUSTOMDIALOG_H
