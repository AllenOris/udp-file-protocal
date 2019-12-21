#ifndef DLG_H
#define DLG_H

#include <QWidget>
#include "header.h"
#include "clientsocket.h"
#include "mkdirdialog.h"

namespace Ui {
class dlg;
}

class dlg : public QWidget
{
    Q_OBJECT

public:
    explicit dlg(QWidget *parent = 0);
    ~dlg();

public slots:
    void on_m_localFolder_returnPressed();

    void on_m_connectButton_clicked();

    void on_m_serverFolder_returnPressed();

    void on_m_disconnectButton_clicked();

    void onDoubleClickLocalView(QModelIndex);
    void onClickLocalView(QModelIndex);
    void onDoubleClickServerView(QModelIndex);
    void onClickServerView(QModelIndex);

    void on_m_refreshButton_clicked();

    void on_m_uploadButton_clicked();

    void on_m_mkdirButton_clicked();

public:
    Ui::dlg *ui;
    QStandardItemModel *model1;
    QStandardItemModel *model2;
    QString downloadFileName;


    //工具
    QIcon getIcon(QStyle::StandardPixmap);
    QString getSize(int sz);
    QDir currentDir;
    QString serverDirStr;
    QString mkdirPath;
    QString joinPath(QString, QString);
    bool checkAuth();


public:
    // 给socket的外部
    void addLog(QString);
    void msgB(QString,QString);
    void onSuccessLogin();
    void onSetLs(QJsonArray&);
    void onSetFolder(QString);
    void onGoodbyeDone();
    void copyFile(QString);

    //socket
    ClientSocket* socket;
private slots:
    void on_m_deleteButton_clicked();
    void on_m_downloadButton_clicked();
    void on_m_localDeleteButton_clicked();
};

#endif // DLG_H
