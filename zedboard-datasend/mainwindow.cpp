#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QtNetwork/QTcpSocket>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnSelFile_clicked()
{
    ui->txtFile->setText(QFileDialog::getOpenFileName());
}

void MainWindow::on_btnSend_clicked()
{
    QFile f(ui->txtFile->text());
    f.open(QIODevice::ReadOnly);
    QByteArray fileData = f.readAll();
    f.close();

    // send write start
    QByteArray cmdData;
    QDataStream cmdStream(&cmdData, QIODevice::WriteOnly);

    cmdStream << (quint32) 0xdeadbeef << ui->txtStartAddr->text().toUInt(0,16);

    QTcpSocket socket;
    socket.connectToHost(ui->txtIPAddr->text(), 80);

    if(!socket.waitForConnected(2000))
    {
        qDebug() << "could not connect to socket";
        return;
    } else
    {
        qDebug() << "connection OK";
    }

    const int packetSize = 256;
    int pos = 0, numChunks = fileData.size() / packetSize;

    qDebug() << "sending " << numChunks << " chunks of " << packetSize << " bytes";


    ui->prgSendProgress->setMaximum(fileData.size());

    int readSize = 0;

    while(pos < fileData.size())
    {
        QByteArray chunk = fileData.mid(pos, packetSize);
        socket.write(chunk);
        socket.waitForBytesWritten(100);
        /*readSize = 0;
        while(readSize < chunk.size())
        {
            readSize += socket.readAll().size();
            socket.waitForReadyRead(100);
            qApp->processEvents();
            qDebug() << "readSize" << readSize;
        }*/
        pos += chunk.size();

        qDebug() << "now at pos " << pos;

        ui->prgSendProgress->setValue(pos);
    }

    qDebug() << "send complete";

    socket.disconnectFromHost();
}
