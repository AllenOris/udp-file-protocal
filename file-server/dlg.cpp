#include "dlg.h"
#include "ui_dlg.h"


dlg::dlg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::dlg)
{
    ui->setupUi(this); setWindowTitle(tr("File Server"));

    userManager=new UserManager(this);
    // 开始监听
    socket=new QUdpSocket();
    bool createSuccess=socket->bind(3600, QUdpSocket::ShareAddress);


    QString msg;
    if(createSuccess)
        msg=tr("[服务] 监听于 ip:%1(%3) 端口:%2").arg(socket->localAddress().toString()).arg(socket->localPort()).arg(getIntranetIP());
    else
        msg=(tr("[错误] 创建端口失败"));
    ui->m_log->append(msg);

    //ui
    ui->m_startButton->setEnabled(true);
    ui->m_stopButton->setEnabled(false);
    ui->m_shiButton->setEnabled(true);
    ui->m_shiStopButton->setEnabled(false);
    isListening=true;
    setWindowTitle(tr("服务端"));

    connect(socket, SIGNAL(readyRead()), this, SLOT(onReceive()));
}

void dlg::onReceive()
{
    while(socket->hasPendingDatagrams()){
        QByteArray datagram;
        datagram.resize(socket->pendingDatagramSize());
        QHostAddress host;
        quint16 port;
        socket->readDatagram(datagram.data(), datagram.size(), &host, &port);
        if(!isListening)continue;
        
        QString source=host.toString()+":"+QString::number(port);
        
        FileThread *thread;
        if(!clientThread.contains(source)){
            thread=clientThread[source]=new FileThread(socket, host, port, userManager);
            connect(thread, &FileThread::appendLog, this, &dlg::appendLog);
            connect(ui->m_shiButton,&QPushButton::clicked, thread, &FileThread::startMiss);
            connect(ui->m_shiStopButton, &QPushButton::clicked, thread, &FileThread::stopMiss);
            thread->start();
        }
        
        thread=clientThread[source];
        thread->read(datagram);
    }
}

void dlg::appendLog(QString msg)
{
    this->ui->m_log->append(msg);
}

dlg::~dlg()
{
    delete ui;
    socket->close();
}

QString dlg::getIntranetIP()
{
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
             return address.toString();
    }
}


void dlg::on_m_startButton_clicked()
{
    ui->m_startButton->setEnabled(false);
    ui->m_stopButton->setEnabled(true);
    isListening=false;
}

void dlg::on_m_stopButton_clicked()
{
    ui->m_startButton->setEnabled(true);
    ui->m_stopButton->setEnabled(false);
    isListening=true;
}

void dlg::on_m_shiButton_clicked()
{
    ui->m_shiButton->setEnabled(false);
    ui->m_shiStopButton->setEnabled(true);
}

void dlg::on_m_shiStopButton_clicked()
{
    ui->m_shiButton->setEnabled(true);
    ui->m_shiStopButton->setEnabled(false);
}
