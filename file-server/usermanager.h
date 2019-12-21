#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include "header.h"

class UserManager : public QObject
{
    Q_OBJECT
public:
    explicit UserManager(QObject *parent = nullptr);

signals:

public slots:
private:
    void createDir(QString);
public:
    QMap<QString, QString> pwdMap;
    QSet<QString> adminSet;
    QString tmpPath;
    QString filePath;

    bool checkAuth(QString, QString);
    bool checkAuthDir(QString, QString);
};

#endif // USERMANAGER_H
