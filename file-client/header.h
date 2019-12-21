#ifndef HEADER_H
#define HEADER_H

#include <QObject>
#include <QMessageBox>
#include <QWidget>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDirIterator>
#include <QDateTime>
#include <QMap>
#include <QDir>
#include <QSet>
#include <QThread>
#include <QHeaderView>
#include <QTableView>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QIcon>
#include <QDialog>
#include <QStyle>
#include <QTimer>
#include <QVector>
#include <QBitArray>
#include <algorithm>
#include <QProcess>
#include <QCoreApplication>
#include <QCryptographicHash>

using namespace  std;

#define WAITING_LOGIN 0x01
#define LOGIN_SUCCESS 0x02
#define WAITING_LS 0x03
#define WAITING_MKDIR 0x04
#define WAITING_UPLOAD 0x05
#define WAITING_DOWNLOAD 0x06
#define WAITING_DELETE 0x07
#define WAITING_SHAKE 0x08
#define WAITING_AUTH 0x09
#define WAITING_FIN1 0x0A
#define WAITING_FIN2 0x0B

#define BAD_REQUEST 501
#define OK 200
#define USER_NOT_EXIST 201
#define WRONG_PASSWORD 202

#endif // HEADER_H
