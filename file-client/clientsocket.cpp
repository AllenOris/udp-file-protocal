#include "clientsocket.h"
#include "dlg.h"

ClientSocket::ClientSocket()
{

}

ClientSocket::ClientSocket(dlg *m_dlg, QObject *parent)
    :m_dlg(m_dlg)
{
    init();
    connect(this, SIGNAL(readyRead()), this, SLOT(onReceive()));
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));
}

void ClientSocket::init()
{
    user="", pwd="", anoy=false;
    seq=0;
    status=0;
    isAuthed=0;
    timer.stop();
}

void ClientSocket::onReceive()
{
    while(hasPendingDatagrams()){
        QByteArray res;
        res.resize(pendingDatagramSize());
        readDatagram(res.data(), res.size());
        QJsonObject data(QJsonDocument::fromJson(res).object());
        checkAck();
        if(status==WAITING_SHAKE)onSendLogin(data);
        else if(status==WAITING_AUTH){onAuthResult(data);}
        else if(status==WAITING_LS){onLsResult(data);}
        else if(status==WAITING_MKDIR){onMkdirResult(data);}
        else if(status==WAITING_DELETE){onDeleteResult(data);}
        else if(status==WAITING_UPLOAD){onUploadResult(data);}
        else if(status==WAITING_DOWNLOAD){onDownloadResult(data);}
        else if(status==WAITING_FIN1){onGoodbyeResult1(data);}
        else if(status==WAITING_FIN2){onGoodbyeResult2(data);}
    }
}



void ClientSocket::shakeHand(QString _host, QString _port, QString _user, QString _pwd, bool _anoy)
{
    host=QHostAddress(_host);
    port=_port.toInt();
    user=_user, pwd=_pwd, anoy=_anoy;
    qDebug()<<host<<port;

    QJsonObject data;
    data.insert("cmd", "hello");
    if(onSendCommand(data)){
        status=WAITING_SHAKE;
        m_dlg->addLog(tr("连接%1:%2中...第一次握手请求...").arg(host.toString()).arg(port));
        uploadBuffer=data;
        timer.start(3000);
    }
}

void ClientSocket::onSendLogin(QJsonObject &res)
{
    if(res.contains("syn")&&res["syn"].toBool()){
        m_dlg->addLog(tr("收到握手确认（第二次握手）"));
        m_dlg->addLog(tr("发送数据（第三次握手），连接成功"));
        timer.stop();
        timerCounts=0;
        m_dlg->addLog(tr("开始验证身份"));
        QJsonObject data;
        if(!anoy)data.insert("user", user);
        else data.insert("user", "<anonymous>");
        data.insert("password", pwd);
        QJsonObject obj;
        obj.insert("data", data);
        obj.insert("cmd", "auth");
        if(onSendCommand(obj)){
            status=WAITING_AUTH;
            //m_dlg->addLog("");
        }
    }
    else{
        m_dlg->addLog("握手失败");
    }
}

void ClientSocket::onUploadFile(QString fileName, QString filePath, QString serverPath)
{
    status = WAITING_UPLOAD;
    uploadFileName=fileName;
    uploadServerPath=serverPath;

    uploadFile.close();
    uploadFile.setFileName(filePath);
    uploadFile.open(QFile::ReadOnly);
    qint64 fileSize=uploadFile.size();
    m_dlg->addLog(tr("开始发送文件，文件大小:%1").arg(m_dlg->getSize(fileSize)));

    //分块
    blockNum=(fileSize-1)/blockSize+1;
    onUploadBlock(blockCurrentNum=1);
}



void ClientSocket::onUploadBlock(int i)
{
    status=WAITING_UPLOAD;
    QString bin=QString(uploadFile.read(blockSize).toBase64());

    QJsonObject obj;
    obj.insert("cmd", "upload");

    QJsonObject data;
    data.insert("name", uploadFileName);
    data.insert("path", uploadServerPath);
    data.insert("num", i);
    data.insert("whole", blockNum);
    data.insert("bin", bin);
    obj.insert("data", data);
    onSendCommand(obj);

    timer.start(3000);
    timerCounts=0;

    uploadBuffer=obj;
}

void ClientSocket::onSendGoodbye()
{
    status=WAITING_FIN1;
    QJsonObject obj;

    obj.insert("cmd", "goodbye");
    onSendCommand(obj);
    m_dlg->addLog(tr("发送断开连接请求（第一次挥手）"));
}

void ClientSocket::onTimer()
{
    if(status==WAITING_SHAKE){
        timer.stop();
        timerCounts++;
        m_dlg->addLog(tr("发送第一次握手超时"));
        if(timerCounts>maxTimerCounts){
            timer.stop();
            m_dlg->addLog(tr("重传次数过多，停止重传"));
            return;
        }

        //超时重传
        m_dlg->addLog(tr("开始重传第%1次").arg(timerCounts));
        timer.start(3000);
        onSendCommand(uploadBuffer);
    }
    else if(status==WAITING_UPLOAD){
        timer.stop();
        timerCounts++;
        m_dlg->addLog(tr("发送%1第%2段数据超时").arg(uploadFileName).arg(blockCurrentNum));
        if(timerCounts>maxTimerCounts){
            timer.stop();
            m_dlg->addLog(tr("重传次数过多，停止重传"));
            return;
        }

        //超时重传
        m_dlg->addLog(tr("开始重传第%1次").arg(timerCounts));
        timer.start(3000);
        onSendCommand(uploadBuffer);
    }
}

bool ClientSocket::onUploadResult(QJsonObject &res)
{
    if(res["status"].toInt()==OK){
        timer.stop();
        m_dlg->addLog(tr("上传进度：%1/%2...").arg(blockCurrentNum).arg(blockNum));
        if(blockCurrentNum==blockNum){
            status=LOGIN_SUCCESS;
            m_dlg->addLog(tr("上传成功"));
            m_dlg->on_m_refreshButton_clicked();
        }
        else{
            onUploadBlock(++blockCurrentNum);
        }
        return true;
    }
    else{
        m_dlg->addLog(tr("上传文件%1/%2 错误：%3").arg(blockCurrentNum).arg(blockNum).arg(res["msg"].toString()));
        status=LOGIN_SUCCESS;
        return false;
    }
}

void ClientSocket::onDownloadFile(QString fileName)
{
    status=WAITING_DOWNLOAD;
    if(downloadFile.isOpen())downloadFile.close();
    downloadFileName=fileName;
    QDir dir(QDir::currentPath());
    if(!dir.exists("tmp"))dir.mkdir("tmp");
    dir.cd("tmp");
    tmpFileName=dir.absolutePath()+"/"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss-zzz");
    if(downloadFile.isOpen())downloadFile.close();
    downloadFile.setFileName(tmpFileName);
    if(!downloadFile.open(QFile::WriteOnly))qDebug()<<tr("open %1 error").arg(downloadFileName);

    QJsonObject obj;
    obj.insert("cmd", "download");
    obj.insert("data", fileName);

    onSendCommand(obj);
    m_dlg->addLog("开始接收文件");
}

bool ClientSocket::checkHash(QJsonObject &data)
{
    QByteArray bin=QByteArray::fromBase64(data["bin"].toString().toUtf8());
    sha256 sha;
    QString hs1=sha.hash(bin);
    QString hs2 = data["sha256"].toString();
    bool eq=hs1==hs2;
    if(eq){
        m_dlg->addLog(tr("校验%1/%2文件， sha256：%3 ...成功").arg(data["num"].toInt()).arg(data["whole"].toInt()).arg(hs1));
        return true;
    }
    else{
       m_dlg->addLog(tr("校验%1/%2文件， sha256：%3 ...失败，接受哈希值为: %4").arg(data["num"].toInt()).arg(data["whole"].toInt()).arg(hs2).arg(hs1));
       return false;
    }
}

bool ClientSocket::onDownloadResult(QJsonObject &res)
{
    if(res["status"].toInt()==OK){
        status=WAITING_DOWNLOAD;
        QJsonObject data=res["data"].toObject();
        QJsonObject obj;

        if(!checkHash(data)){
            obj.insert("cmd", "resend");
            onSendCommand(obj);
            m_dlg->addLog("请求重传");
            return false;
        }


        int whole=data["whole"].toInt();
        int num=data["num"].toInt();
        if(num==1){
            rcvBlockNum=whole;
            rcvBlockCurrentNum=0;
        }
        if(num==rcvBlockCurrentNum+1){
            rcvBlockCurrentNum=num;
            QByteArray bin=QByteArray::fromBase64(data["bin"].toString().toUtf8());
            downloadFile.write(bin.data(), bin.size());
        }

        m_dlg->addLog(tr("下载文件%1/%2...成功").arg(rcvBlockCurrentNum).arg(rcvBlockNum));

        if(rcvBlockCurrentNum==rcvBlockNum){
            downloadFile.close();
            m_dlg->copyFile(tmpFileName);
            m_dlg->addLog("接收文件成功");
            status=LOGIN_SUCCESS;
        }
        obj.insert("cmd", "ack");
        onSendCommand(obj);
    }
    else{
        m_dlg->addLog(tr("下载文件 错误：%1").arg(res["msg"].toString()));
        status=LOGIN_SUCCESS;
        return false;
    }
}

bool ClientSocket::onGoodbyeResult1(QJsonObject &res)
{
    if(res["status"].toInt()==OK){
        status=WAITING_FIN2;
        m_dlg->addLog(tr("收到服务端挥手确认（第二次挥手）"));
    }
    else{
        m_dlg->addLog(tr("挥手失败 错误：%1").arg(res["msg"].toString()));
        status=LOGIN_SUCCESS;
        return false;
    }
}

bool ClientSocket::onGoodbyeResult2(QJsonObject &res)
{
    if(res["status"].toInt()==OK){
        status=0;
        m_dlg->addLog(tr("收到服务端挥手指令（第三次挥手）"));
        QJsonObject obj;
        obj.insert("cmd","ack");
        onSendCommand(obj);
        m_dlg->addLog(tr("发送挥手确认（第四次挥手）"));
        m_dlg->onGoodbyeDone();
    }
    else{
        m_dlg->addLog(tr("挥手失败 错误：%1").arg(res["msg"].toString()));
        status=LOGIN_SUCCESS;
        return false;
    }

}

void ClientSocket::onSendMkdir(QString path, QString name)
{
    status=WAITING_MKDIR;
    QJsonObject obj;
    QJsonObject data;
    obj.insert("cmd", "mkdir");
    data.insert("path", path);
    data.insert("name", name);
    obj.insert("data", data);
    onSendCommand(obj);
    m_dlg->addLog("创建目录...");
}

void ClientSocket::onSendDelete(QString path)
{
    status=WAITING_DELETE;
    QJsonObject obj;
    QJsonObject data;
    obj.insert("cmd", "delete");
    data.insert("path", path);
    obj.insert("data", data);
    onSendCommand(obj);
}



bool ClientSocket::onAuthResult(QJsonObject &res)
{
    if(res.contains("status")&&res["status"].toInt()==200){
        m_dlg->addLog("登陆成功");
        m_dlg->onSuccessLogin();
        m_dlg->onSetFolder("/");
        status=LOGIN_SUCCESS;
        isAuthed=true;
        onSendLs("/");
    }
    else{
        m_dlg->addLog(tr("登录失败 错误：%1").arg(res["msg"].toString()));
        status=0;
    }
}

bool ClientSocket::onDeleteResult(QJsonObject &res)
{
    status=LOGIN_SUCCESS;
    if(res["status"].toInt()==OK){
        m_dlg->addLog(tr("删除成功"));
        m_dlg->on_m_refreshButton_clicked();
        return true;
    }
    else{
        m_dlg->addLog(tr("删除失败 错误：%1").arg(res["msg"].toString()));
        return false;
    }
}

bool ClientSocket::onLsResult(QJsonObject &res)
{
    if(res["status"].toInt()==OK){
        m_dlg->onSuccessLogin();
        status=LOGIN_SUCCESS;
        QJsonArray files=res["data"].toArray();
        m_dlg->onSetLs(files);
        return true;
    }
    else{
        m_dlg->addLog(tr("ls获取失败 错误：%1").arg(res["msg"].toString()));
        status=LOGIN_SUCCESS;
        return false;
    }
}


bool ClientSocket::onMkdirResult(QJsonObject &res)
{
    status=LOGIN_SUCCESS;
    if(res["status"].toInt()==OK){
        m_dlg->addLog(tr("创建目录成功"));
        m_dlg->on_m_refreshButton_clicked();
        return true;
    }
    else{
        m_dlg->addLog(tr("创建目录失败 错误：%1").arg(res["msg"].toString()));
        return false;
    }
}




bool ClientSocket::checkAck()
{

}

bool ClientSocket::onCheckAuth()
{
    if(status>=LOGIN_SUCCESS)return true;
    else{
        m_dlg->msgB(tr("失败的请求"),tr("未登录"));
        return false;
    }
}

void ClientSocket::onSendLs(QString dir)
{
    if(!onCheckAuth())return;
    status=WAITING_LS;
    QJsonObject obj;
    obj.insert("cmd", "ls");
    obj.insert("data", dir);
    if(onSendCommand(obj)){
        m_dlg->addLog(tr("发送获取[%1]目录请求").arg(dir));
    }
    else{
        m_dlg->addLog("发送ls命令失败");
    }
}

bool ClientSocket::onSendCommand(QJsonObject &obj)
{
    obj.insert("seq", seq++);
    obj.insert("ack", ++lastAck);
    QJsonDocument doc(obj);
    QByteArray data=doc.toJson(QJsonDocument::Compact);
    int size=writeDatagram(data, data.size(), host, port);
    if(size==-1){
        m_dlg->addLog(tr("发送错误 错误代码：%1 错误信息：%2").arg(this->error()).arg(this->errorString()));
        qDebug()<<"send error"<<m_dlg->getSize(data.size());
        return false;
    }
    else return true;
}



