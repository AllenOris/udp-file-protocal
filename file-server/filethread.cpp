#include "filethread.h"

FileThread::FileThread()
{

}

FileThread::FileThread(QUdpSocket *socket, QHostAddress addr, quint16 port, UserManager* userManager, QObject *parent)
    :socket(socket),
      addr(addr),
      port(port),
      userManager(userManager)
{
}

void FileThread::init()
{
    shakedHand=0;
    isAuth=0;
    isGoodbye=0;
    timer.stop();
    timerCounts=0;
}

void FileThread::onLs(QJsonValue val)
{
    QDir dir;
    QJsonObject obj;
    QJsonArray data;
    if(!userManager->checkAuth(userName, val.toString())){
        onSendCommand(obj, BAD_AUTH, tr("无权限"));
    }
    else if(!dir.cd(userManager->filePath+val.toString())){
        onSendCommand(obj, BAD_REQUEST, tr("目录不存在"));
    }
    else{
        for(auto info:dir.entryInfoList(QDir::AllEntries|QDir::NoDot)){
            QString name=info.fileName();
            QString size=getSize(info.size());
            QString time=info.lastModified().toString("yyyy-MM-dd HH:mm:ss");
            QJsonObject row;
            row.insert("name", name);
            row.insert("size", info.isDir()?"":size);
            row.insert("time", time);
            row.insert("isFolder", info.isDir());
            data.append(row);
        }
        obj.insert("data", data);
        onSendCommand(obj);
    }
}

void FileThread::onMkdir(QJsonValue val)
{
    QString path=val.toObject()["path"].toString();
    QString name=val.toObject()["name"].toString();
    QJsonObject obj;
    if(path.isEmpty()||name.isEmpty())onSendCommand(obj, BAD_REQUEST, "请求不合法");
    else if(!userManager->checkAuthDir(userName, path))onSendCommand(obj, BAD_REQUEST, "该目录无权限");
    else{
        QDir dir;
        if(dir.cd(userManager->filePath+path)&&dir.mkdir(name))onSendCommand(obj);
        else onSendCommand(obj, BAD_REQUEST, "服务器创建失败");
    }
}

void FileThread::onUpload(QJsonValue val)
{
    QJsonObject obj;
    QJsonObject data=val.toObject();

    int num=data["num"].toInt();
    int whole=data["whole"].toInt();

    if(num==1){
        QString name=data["name"].toString();
        QString path=data["path"].toString();

        if(!userManager->checkAuthDir(userName, path))onSendCommand(obj, BAD_REQUEST, "该目录无权限");

        uploadFilePath=joinPath(userManager->filePath+path, name);
        tmpFileName = userManager->tmpPath+"/"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss-zzz");

        if(uploadFile.isOpen())uploadFile.close();
        uploadFile.setFileName(tmpFileName);
        uploadFile.open(QFile::WriteOnly);

        blockNum=whole;
        blockCurrentNum=0;
    }

    onSendCommand(obj);

    if(num==blockCurrentNum+1){
        blockCurrentNum=num;
        QByteArray bin=QByteArray::fromBase64(data["bin"].toString().toUtf8());
        uploadFile.write(bin.data(), bin.size());
    }

    if(blockCurrentNum==blockNum){
        uploadFile.close();
        if(QFile::copy(tmpFileName, uploadFilePath)){
            qDebug()<<tmpFileName<<uploadFilePath<<"复制成功";
        }
        else{
            qDebug()<<tmpFileName<<uploadFilePath<<"复制失败";
        }
    }
}

void FileThread::onDownload(QJsonValue val)
{
    QJsonObject obj;
    QString name=val.toString();
    qDebug()<<name;
    if(!userManager->checkAuth(userName, name)){
        onSendCommand(obj, BAD_REQUEST, "无权限下载该文件");
        return;
    }

    senderFileName=userManager->filePath+name;
    senderFile.setFileName(senderFileName);

    qDebug()<<"d file:"<<senderFileName;

    if(senderFile.isOpen())senderFile.close();
    
    if(!senderFile.open(QFile::ReadOnly)){
        onSendCommand(obj, BAD_REQUEST, "无法访问文件");
        return;
    }

    qint64 fileSize=senderFile.size();

    //分块
    senderBlockNum=(fileSize-1)/blockSize+1;
    onSendBlock(senderBlockCurrentNum=1);
}

void FileThread::onSendBlock(int i)
{
    QByteArray byteArr=senderFile.read(blockSize);
    QString bin=QString(byteArr.toBase64());

    QJsonObject obj;
    sha256 sha2;
    QString hs=sha2.hash(byteArr);

    QJsonObject data;
    data.insert("num", i);
    data.insert("whole", senderBlockNum);
    data.insert("sha256", hs);
    
    data.insert("bin", bin);
    obj.insert("data", data);


    senderBuffer=obj;

    if(miss){
        qDebug()<<data;
        addLog("模拟失真中");
        data.insert("bin", "aa"+bin.right(bin.length()-2));
        qDebug()<<data;
        obj.insert("data", data);
    }

    onSendCommand(obj);

    timerCounts=0;
    timer.start(3000);
    
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));

    addLog(tr("发送文件%1/%2, sha256值为%3").arg(i).arg(senderBlockNum).arg(data["sha256"].toString()));
}

void FileThread::startMiss()
{

    miss=true;
}

void FileThread::stopMiss()
{
    miss=false;
}

void FileThread::onTimer()
{
    timer.stop();
    timerCounts++;
    qDebug()<<(tr("发送%1第%2段数据超时").arg(senderFileName).arg(senderBlockCurrentNum));
    
    if(timerCounts>maxTimerCounts){
        timer.stop();
        qDebug()<<(tr("重传次数过多，停止重传"));
        return;
    }

    //超时重传
    qDebug()<<(tr("开始重传第%1次").arg(timerCounts));
    timer.start(3000);
    onSendCommand(senderBuffer);
}



void FileThread::onAck(QJsonObject obj)
{
    timer.stop();
    if(senderBlockCurrentNum<senderBlockNum){
        onSendBlock(++senderBlockCurrentNum);
    }
}

void FileThread::onResend(QJsonObject obj)
{
   timer.stop();
   addLog(tr("对方校验出错，开始重发"), 1);
   onSendCommand(senderBuffer);
   timer.start(3000);
}

void FileThread::onGoodBye1(QJsonObject)
{
    QJsonObject obj;
    onSendCommand(obj);
    obj.insert("fin",true);
    onSendCommand(obj);
    isGoodbye=1;
}

void FileThread::onGooedBye2(QJsonObject)
{
    //断开连接
    init();
}




void FileThread::onDelete(QJsonValue val)
{
    QString path=val.toObject()["path"].toString();
    QJsonObject obj;
    if(path.isEmpty())onSendCommand(obj, BAD_REQUEST, "请求不合法");
    else if(!userManager->checkAuthDir(userName, path))onSendCommand(obj, BAD_REQUEST, "该目录无权限");
    else{
        path=userManager->filePath+path;
        bool success=false;
        QFileInfo fileInfo(path);
        qDebug()<<"file:"<<path<<fileInfo.isFile();
        if(fileInfo.isFile()){
            QDir dir;
            qDebug()<<fileInfo.absoluteFilePath();
            success=dir.remove(fileInfo.absoluteFilePath());
        }
        else{
            QDir dir(path);
            success=dir.removeRecursively();
        }
        if(success)onSendCommand(obj);
        else onSendCommand(obj, BAD_REQUEST, "服务器无法删除该文件");
    }
}


void FileThread::onAuth(QJsonObject obj)
{
    QJsonObject res;
    if(obj.contains("data")){
        QJsonObject data=obj["data"].toObject();
        QString user=data["user"].toString();
        QString pwd=data["password"].toString();
        if(user!="<anonymous>"&&!userManager->pwdMap.contains(user)){
            onSendCommand(res, USER_NOT_EXIST, tr("用户不存在"));
        }
        else if(user!="<anonymous>"&&userManager->pwdMap[user]!=pwd){
            onSendCommand(res, WRONG_PASSWORD, tr("密码错误"));
        }
        else{
            userName=user;
            isAuth=true;
            onSendCommand(res);
        }
    }
    else{
        onSendCommand(res, BAD_REQUEST, "BAD REQUEST");
    }
}



void FileThread::onShakeHand(QJsonValue val)
{
    init();
    shakedHand++;
    QJsonObject data;
    data.insert("syn", true);
    onSendCommand(data, 200);
    addLog("握手成功");
}


void FileThread::onSendCommand(QJsonObject obj, int status, QString errMsg)
{
    obj.insert("ack", ++lastAck);
    obj.insert("seq", seq++);
    obj.insert("status", status);
    obj.insert("msg", errMsg);
    QJsonDocument doc(obj);
    QByteArray data=doc.toJson(QJsonDocument::Compact);

    QString msg;

    if(obj.contains("status")&&obj.contains("msg")){
        msg+=tr(" [状态码]:%1, [消息]:\"%2\"  [seq]:%3").arg(obj["status"].toInt()).arg(obj["msg"].toString()).arg(seq);
    }

    socket->writeDatagram(data, data.size(),addr,port);

    addLog(msg, 2);
}

QString FileThread::getSize(int sz)
{
    float size=sz;
    if(size<1024)return QString::number(size,'f',0)+"B";
    size/=1024;
    if(size<1024)return QString::number(size,'f',2)+"KB";
    size/=1024;
    if(size<1024)return QString::number(size,'f',2)+"MB";
    size/=1024;
    if(size<1024)return QString::number(size,'f',2)+"GB";
}



void FileThread::run()
{
    this->exec();
}

void FileThread::read(QByteArray b)
{
    QJsonObject data(QJsonDocument::fromJson(b).object());
    QJsonValue d=data["data"];
    QString cmd=data["cmd"].toString();
    addLog(tr("[请求]:%1").arg(cmd), 1);

    if(!shakedHand||cmd=="hello")onShakeHand(d);
    else if(cmd=="goodbye")onGoodBye1(data);

    else if(isGoodbye&&cmd=="ack")onGoodBye1(data);
    else if(cmd==tr("mkdir"))onMkdir(d);
    else if(cmd=="ls")onLs(d);
    else if(cmd=="upload")onUpload(d);
    else if(cmd=="download")onDownload(d);
    else if(cmd=="delete")onDelete(d);
    else if(cmd=="auth")onAuth(data);
    else if(cmd=="ack")onAck(data);
    else if(cmd=="resend")onResend(data);

}


QString FileThread::joinPath(QString a, QString b)
{
    if(a.endsWith('/'))a+=b;
    else a+="/"+b;
    if(a.endsWith("..")){
        a=a.replace("/..","");
        while(!a.isEmpty()&&a.at(a.length()-1)!=QChar('/')){
            a=a.left(a.length()-1);
        }
        if(a.length()>1) a=a.left(a.length()-1);
    }
    return a;
}

void FileThread::addLog(QString msg, int type)
{
    QString log;
    QString rcv;
    rcv=tr("连接对象%1:%2 ").arg(addr.toString()).arg(port);
    if(type==0){
        log+=tr("[服务] ");
    }
    else if(type==1){
        log+=tr("[接受] ");
    }
    else if(type==2){
        log+=tr("[发送] ");
    }
    if(type)log+=rcv;
    log+=msg;
    emit(appendLog(log));
}

