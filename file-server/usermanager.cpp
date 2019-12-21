#include "usermanager.h"
#include "sha256.h"

UserManager::UserManager(QObject *parent) : QObject(parent)
{
    // 读取配置文件
    QFile file(":/res/config.json");

    QString val;
    if(!file.open(QFile::ReadOnly|QIODevice::Text))qDebug()<<"read config.json error!";
    else{
        val=file.readAll();
        file.close();

        QJsonObject d = QJsonDocument::fromJson(val.toUtf8()).object();

        // 获取根目录存储路径
        tmpPath=d["tmpFilePath"].toString();
        filePath=d["filePath"].toString();

        // 获取用户信息
        QJsonArray users=d["users"].toArray();
        QJsonArray admin=d["admin"].toArray();

        for(auto p:users){
            QString name=p.toObject()["name"].toString();
            QString password=p.toObject()["password"].toString();
            pwdMap[name]=password;
        }
        for(auto p:admin){
            QString adminName=p.toVariant().toString();
            adminSet.insert(adminName);
        }

        // 创建目录
        for(auto p:pwdMap)createDir(p);
        createDir("shares");
    }
}

void UserManager::createDir(QString name)
{
    QDir dir;
    QString dirStr=filePath+"/"+name;
    if(!dir.exists(dirStr)){
        if(!dir.mkpath(dirStr)){
            qDebug()<<tr("创建%1目录失败").arg(dirStr);
        }
    }
}

bool UserManager::checkAuth(QString userName, QString path)
{
    if(adminSet.contains(userName))return true;
    if(path.startsWith("/shares")||path=="/")return true;
    if(path.startsWith("/"+userName+"/")||path=="/"+userName)return true;
    return false;
}

bool UserManager::checkAuthDir(QString userName, QString path)
{
    if(adminSet.contains(userName))return true;
    if(path=="/")return false;
    if(path.startsWith("/shares"))return true;
    if(path.startsWith("/"+userName+"/")||path=="/"+userName)return true;
    return false;
}
