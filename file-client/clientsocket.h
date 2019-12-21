#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include "header.h"
#include "sha256.h"

class dlg;

class ClientSocket : public QUdpSocket
{
    Q_OBJECT
public:
    ClientSocket();
    explicit ClientSocket(dlg* m_dlg, QObject *parent=NULL);

private:
    dlg*m_dlg;


private slots:
    void onReceive();
    void onTimer();

public:
    // 通信
    void shakeHand(QString, QString, QString, QString, bool);
    void init();

public:

    // 计时器
    QTimer timer;
    int timerCounts=0;
    int maxTimerCounts=3;

    // 客户信息
    bool anoy;
    QString user;
    QString pwd;
    QHostAddress host;
    quint16 port;

    //通信
    int seq;
    int lastAck;
    bool isAuthed=0;

    //状态
    int status=0;

    //文件发送
    QFile uploadFile;
    QJsonObject uploadBuffer;
    QString uploadServerPath;
    QString uploadFileName;
    int blockSize=16384;
    int blockNum;
    int blockCurrentNum;

    //文件接收
    QString downloadFileName;
    QString tmpFileName;
    QFile downloadFile;
    int rcvBlockCurrentNum;
    int rcvBlockNum;

    void onSendLogin(QJsonObject&);
    void onUploadFile(QString, QString, QString);
    void onDownloadFile(QString);
    void onSendMkdir(QString, QString);
    void onSendDelete(QString);
    void onUploadBlock(int);
    void onSendGoodbye();
    bool onSendCommand(QJsonObject&);

    bool onMkdirResult(QJsonObject&);
    bool onAuthResult(QJsonObject&);
    bool onDeleteResult(QJsonObject&);
    bool onLsResult(QJsonObject&);
    bool onUploadResult(QJsonObject&);
    bool onDownloadResult(QJsonObject&);
    bool onGoodbyeResult1(QJsonObject&);
    bool onGoodbyeResult2(QJsonObject&);

    // check
    bool onCheckAuth();
    bool checkAck();
    bool checkHash(QJsonObject&);
    void onSendLs(QString dir);


};

#endif // CLIENTSOCKET_H
