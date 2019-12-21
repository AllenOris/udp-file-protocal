#ifndef FILETHREAD_H
#define FILETHREAD_H

#include "header.h"
#include "usermanager.h"
#include "sha256.h"

class FileThread : public QThread
{
    Q_OBJECT
public:
    FileThread();
    FileThread(QUdpSocket*socket, QHostAddress addr, quint16 port, UserManager *userManager, QObject *parent=NULL);
    void init();

private:

    //计时器
    QTimer timer;
    int timerCounts=0;
    int maxTimerCounts=3;

    //socket
    QUdpSocket *socket;
    QHostAddress addr;
    quint16 port;
    UserManager *userManager;

    // 收到的文件
    QFile uploadFile;
    QString uploadFilePath;
    QString tmpFileName;
    int blockNum;
    int blockCurrentNum;

    //发送的文件
    QFile senderFile;
    QString senderFileName;
    QJsonObject senderBuffer;
    int blockSize=16384;
    int senderBlockNum;
    int senderBlockCurrentNum;

    //state
    int shakedHand=0;
    bool isAuth=0;
    bool isGoodbye=0;

    //flag
    int seq;
    int lastAck;

    //user
    QString userName;

    //miss
    bool miss=0;

    //function
    void onLs(QJsonValue val);
    void onMkdir(QJsonValue val);
    void onUpload(QJsonValue val);
    void onDownload(QJsonValue val);
    void onDelete(QJsonValue val);
    void onShakeHand(QJsonValue val);
    void onAuth(QJsonObject obj);
    void onAck(QJsonObject obj);
    void onResend(QJsonObject obj);
    void onGoodBye1(QJsonObject obj);
    void onGooedBye2(QJsonObject obj);

    void onSendCommand(QJsonObject, int status=200, QString msg=tr("ok"));
    void onSendBlock(int);
    QString getSize(int size);

    //工具
    QString joinPath(QString, QString);
    void addLog(QString, int type=0);

signals:
    void appendLog(QString);

public slots:
    void run();
    void read(QByteArray);
    void onTimer();
    void startMiss();
    void stopMiss();


};

#endif // FILETHREAD_H
