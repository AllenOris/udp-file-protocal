#include "dlg.h"
#include "ui_dlg.h"

dlg::dlg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::dlg)
{
    ui->setupUi(this);

    // 初始化ui
    ui->m_connectType->addItem(tr("密码登录"));
    ui->m_connectType->addItem(tr("匿名登录"));

    model1 = new QStandardItemModel(this);
    model1->setColumnCount(3);
    model1->setHeaderData(0,Qt::Horizontal,tr("文件名"));
    model1->setHeaderData(1,Qt::Horizontal,tr("大小"));
    model1->setHeaderData(2,Qt::Horizontal,tr("修改时间"));
    ui->m_localView->setModel(model1);
    ui->m_localView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(ui->m_localView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDoubleClickLocalView(QModelIndex)));
    connect(ui->m_localView, SIGNAL(clicked(QModelIndex)), this, SLOT(onClickLocalView(QModelIndex)));

    model2 = new QStandardItemModel(this);
    model2->setColumnCount(3);
    model2->setHeaderData(0,Qt::Horizontal,tr("文件名"));
    model2->setHeaderData(1,Qt::Horizontal,tr("大小"));
    model2->setHeaderData(2,Qt::Horizontal,tr("修改时间"));
    ui->m_serverView->setModel(model2);
    ui->m_serverView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(ui->m_serverView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDoubleClickServerView(QModelIndex)));
    connect(ui->m_serverView, SIGNAL(clicked(QModelIndex)), this, SLOT(onClickServerView(QModelIndex)));


    ui->m_localFolder->setText(tr("/home/allen/桌面/code/web/work/file"));
    on_m_localFolder_returnPressed();
    ui->m_disconnectButton->setEnabled(false);

    setWindowTitle(tr("客户端"));


    //创建socket
    socket=new ClientSocket(this,this);
    socket->bind(0, QUdpSocket::ShareAddress);
    addLog(tr("Socket开启，端口号%1").arg(socket->localPort()));
}

dlg::~dlg()
{
    delete ui;
}

void dlg::on_m_localFolder_returnPressed()
{
    QString strDir=ui->m_localFolder->text();
    QDir dir;
    if(dir.exists(strDir)){
        dir.cd(strDir);
        currentDir=dir;
        QStandardItemModel* model=model1;
        model->removeRows(0,model->rowCount());

        QIcon folderIcon=getIcon(QStyle::SP_FileDialogNewFolder);
        QIcon fileIcon=getIcon(QStyle::SP_FileIcon);
        for(auto info:dir.entryInfoList(QDir::Dirs|QDir::NoDot)){
            QList<QStandardItem*>rowItem;
            QString name=info.fileName();
            QString size=getSize(info.size());
            QString time=info.lastModified().toString("yyyy-MM-dd HH:mm:ss");
            rowItem.append(new QStandardItem(name));
            rowItem.append(new QStandardItem(""));
            rowItem.append(new QStandardItem(time));
            rowItem.front()->setIcon(folderIcon);
            model->appendRow(rowItem);
        }
        for(auto info:dir.entryInfoList(QDir::Files)){
            QList<QStandardItem*>rowItem;
            QString name=info.fileName();
            QString size=getSize(info.size());
            QString time=info.lastModified().toString("yyyy-MM-dd HH:mm:ss");
            rowItem.append(new QStandardItem(name));
            rowItem.append(new QStandardItem(size));
            rowItem.append(new QStandardItem(time));
            rowItem.front()->setIcon(fileIcon);
            model->appendRow(rowItem);
        }
    }
}

QString dlg::getSize(int sz)
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

QIcon dlg::getIcon(QStyle::StandardPixmap x)
{
    QStyle* style = QApplication::style();
    QIcon icon = style->standardIcon(x);
    return icon;
}

QString dlg::joinPath(QString a, QString b)
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

bool dlg::checkAuth()
{
    if(socket->isAuthed)return true;
    msgB("错误","请先登录");
    return false;
}



void dlg::msgB(QString title, QString msg)
{
    QMessageBox* box=new QMessageBox();
    box->setWindowTitle(title);
    box->setText(msg);
    box->setWindowIcon(getIcon(QStyle::SP_MessageBoxWarning));
    box->setIconPixmap(getIcon(QStyle::SP_MessageBoxWarning).pixmap(50, 50));
    box->show();
}

void dlg::addLog(QString msg)
{
    msg=tr("状态：")+msg;
    ui->m_log->append(msg);
}

void dlg::on_m_connectButton_clicked()
{

    QString host=ui->m_host->text();
    QString port=ui->m_port->text();
    QString name=ui->m_name->text();
    QString pwd=ui->m_password->text();
    int index=ui->m_connectType->currentIndex();

    if(host.isEmpty()||port.isEmpty())msgB(tr("语法错误"), tr("无法识别的主机地址和端口号"));
    else if(name.isEmpty()&&index==0)msgB(tr("登录失败"), tr("用户名为空，可以选择使用匿名登录"));
    else socket->shakeHand(host, port, name, pwd, index);
}

void dlg::onSetLs(QJsonArray &arr)
{

    serverDirStr=ui->m_serverFolder->text();

    QStandardItemModel*model=model2;
    model->clear();


    model2->setHeaderData(0,Qt::Horizontal,tr("文件名"));
    model2->setHeaderData(1,Qt::Horizontal,tr("大小"));
    model2->setHeaderData(2,Qt::Horizontal,tr("修改时间"));

    QIcon folderIcon=getIcon(QStyle::SP_FileDialogNewFolder);
    QIcon fileIcon=getIcon(QStyle::SP_FileIcon);

    for(auto info:arr){
        QList<QStandardItem*>rowItem;

        QString name=info.toObject()["name"].toString();
        QString size=info.toObject()["size"].toString();
        bool isFolder=info.toObject()["isFolder"].toBool();
        QString time=info.toObject()["time"].toString();

        rowItem.append(new QStandardItem(name));
        rowItem.append(new QStandardItem(size));
        rowItem.append(new QStandardItem(time));

        if(isFolder)rowItem.front()->setIcon(folderIcon);
        else rowItem.front()->setIcon(fileIcon);

        model->appendRow(rowItem);
    }
}

void dlg::onSetFolder(QString strDir)
{
    ui->m_serverFolder->setText(strDir);
}


void dlg::copyFile(QString tmpFile)
{
    QString name=currentDir.absolutePath()+"/"+downloadFileName;
    qDebug()<<"复制文件"<<tmpFile<<name;
    QFile::copy(tmpFile, name);
    on_m_localFolder_returnPressed();
}

void dlg::on_m_serverFolder_returnPressed()
{
    socket->onSendLs(ui->m_serverFolder->text());
}


void dlg::onGoodbyeDone()
{
     addLog("已断开连接");

     //按钮重置
    ui->m_connectButton->setEnabled(true);
    ui->m_disconnectButton->setEnabled(false);
    model2->clear();

    socket->init();


}

void dlg::on_m_disconnectButton_clicked()
{
    //挥手
    socket->onSendGoodbye();
}

void dlg::onDoubleClickLocalView(QModelIndex index)
{
    QStandardItem *item=model1->itemFromIndex(index);
    QString folder=item->text();
    QDir dir;
    if(dir.cd(currentDir.absolutePath()+"/"+folder)){
        qDebug()<<"目录切换成功"<<dir.absolutePath();
        currentDir=dir;
        ui->m_localFolder->setText(dir.absolutePath());
        on_m_localFolder_returnPressed();
    }
    else{
        addLog("无效的路径");
    }
}

void dlg::onClickLocalView(QModelIndex index)
{
    QStandardItem *item=model1->itemFromIndex(index);
    ui->m_localFolder->setText(currentDir.absolutePath()+"/"+item->text());
}

void dlg::onDoubleClickServerView(QModelIndex index)
{
    QStandardItem *item=model2->itemFromIndex(index);
    ui->m_serverFolder->setText(joinPath(serverDirStr, item->text()));
    on_m_serverFolder_returnPressed();
}

void dlg::onClickServerView(QModelIndex index)
{
    QStandardItem *item=model2->itemFromIndex(index);
    downloadFileName=item->text();
    ui->m_serverFolder->setText(joinPath(serverDirStr, item->text()));
}

void dlg::onSuccessLogin()
{
    ui->m_connectButton->setEnabled(false);
    ui->m_disconnectButton->setEnabled(true);
}



void dlg::on_m_refreshButton_clicked()
{
    addLog("刷新目录...");
    ui->m_serverFolder->setText(serverDirStr);
    on_m_serverFolder_returnPressed();
}

void dlg::on_m_uploadButton_clicked()
{
    if(!checkAuth())return;
    QString filePath=ui->m_localFolder->text();
    QFileInfo info(filePath);
    if(!info.isFile())msgB("上传失败", "请选择一个文件上传");
    else socket->onUploadFile(info.fileName(), filePath, serverDirStr);
}

void dlg::on_m_mkdirButton_clicked()
{
    if(!checkAuth())return;
    MkdirDialog dlg(this);
    if(dlg.exec() == QDialog::Accepted)
    {
        socket->onSendMkdir(serverDirStr, mkdirPath);
    }
}

void dlg::on_m_deleteButton_clicked()
{
    if(!checkAuth())return;
    if (QMessageBox::Yes == QMessageBox::question(this,
                                                  tr("删除确定"),
                                                  tr("确定删除%1, 文件无法恢复").arg(ui->m_serverFolder->text())),
                                                  QMessageBox::Yes | QMessageBox::No,
                                                  QMessageBox::Yes) {
        addLog("删除文件中...");
        socket->onSendDelete(ui->m_serverFolder->text());
    }
}

void dlg::on_m_downloadButton_clicked()
{
    if(!checkAuth())return;
    QString fileName=ui->m_serverFolder->text();
    socket->onDownloadFile(fileName);
}

void dlg::on_m_localDeleteButton_clicked()
{
    currentDir.remove(ui->m_localFolder->text());
    ui->m_localFolder->setText(currentDir.absolutePath());
    this->on_m_localFolder_returnPressed();
}
