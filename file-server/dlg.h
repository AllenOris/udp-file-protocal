#ifndef DLG_H
#define DLG_H

#include "header.h"
#include "usermanager.h"
#include "filethread.h"

namespace Ui {
class dlg;
}

class dlg : public QWidget
{
    Q_OBJECT

public:
    explicit dlg(QWidget *parent = 0);
    ~dlg();

private slots:
    void onReceive();
    void appendLog(QString);

    void on_m_startButton_clicked();

    void on_m_stopButton_clicked();

    void on_m_shiButton_clicked();

    void on_m_shiStopButton_clicked();

private:
    Ui::dlg *ui;
    QUdpSocket *socket;
    QString getIntranetIP();
    UserManager* userManager;
    QMap<QString, FileThread*>clientThread;
    bool isListening=true;
};

#endif // DLG_H
