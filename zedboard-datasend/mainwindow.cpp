#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDebug>
#include <QDateTime>
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

    // copy file size into UI text field for convenience
    QFileInfo f(ui->txtFile->text());

    if(f.size() % 4 != 0) {
      ui->txtSize->setText("Trouble! mod 4 != 0");
    } else {
      ui->txtSize->setText(QString::number(f.size(), 16));
    }
}

void MainWindow::on_btnSend_clicked()
{
    QString fileName = ui->txtFile->text();
    unsigned int startAddr = ui->txtStartAddr->text().toUInt(0, 16);
    unsigned int dataSize = ui->txtSize->text().toUInt(0, 16);

    writeFromFile(startAddr, dataSize, fileName);
}

QByteArray MainWindow::makeReadCommand(unsigned int addr, unsigned int size)
{
  QString cmd = "r " + QString::number(addr, 16)+ " " + QString::number(size,16) + "\n";

  return cmd.toLocal8Bit();
}

QByteArray MainWindow::makeWriteCommand(unsigned int addr, unsigned int size)
{
  QString cmd = "w " + QString::number(addr, 16)+ " " + QString::number(size,16) + "\n";

  return cmd.toLocal8Bit();
}

void MainWindow::on_btnReceive_clicked()
{
    unsigned int startAddr = ui->txtStartAddr->text().toUInt(0, 16);
    unsigned int dataSize = ui->txtSize->text().toUInt(0, 16);
    readToFile(startAddr, dataSize, ui->txtFile->text());
}

bool MainWindow::writeFromFile(unsigned int startAddr, unsigned int dataSize, QString fileName)
{
    QFile f(fileName);
    f.open(QIODevice::ReadOnly);
    QByteArray fileData = f.readAll();
    f.close();

    QTcpSocket socket;
    socket.connectToHost(ui->txtIPAddr->text(), 80);

    if(!socket.waitForConnected(2000))
    {
        qDebug() << "could not connect to socket";
        return false;
    } else
    {
        qDebug() << "connection OK";
    }

    // build command and send
    socket.write(makeWriteCommand(startAddr, dataSize));
    socket.waitForBytesWritten(100);

    const int packetSize = 256;
    int pos = 0, numChunks = fileData.size() / packetSize;

    if(fileData.size() % 4 != 0) {
      qDebug() << "error: file size must be divisable by 4";
      socket.disconnectFromHost();
      return false;
    }

    qDebug() << "sending " << numChunks << " chunks of " << packetSize << " bytes";


    ui->prgSendProgress->setMaximum(fileData.size());

    quint64 start = QDateTime::currentMSecsSinceEpoch();
    while(pos < fileData.size())
    {
        QByteArray chunk = fileData.mid(pos, packetSize);
        socket.write(chunk);
        socket.waitForBytesWritten(100);

        pos += chunk.size();

        //qDebug() << "now at pos " << pos;

        ui->prgSendProgress->setValue(pos);
    }
    quint64 end = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "send completed in " << end - start << " msecs";
    qDebug() << "Bandwidth: " << (fileData.size()/1024) / ((end-start)/1000) << " KB/s" << endl;

    socket.waitForReadyRead(100);

    socket.disconnectFromHost();

    return true;
}

bool MainWindow::readToFile(unsigned int startAddr, unsigned int dataSize, QString fileName)
{
    qDebug() << "Does not work yet!";

    return false;

    QTcpSocket socket;
    socket.connectToHost(ui->txtIPAddr->text(), 80);

    if(!socket.waitForConnected(2000))
    {
        qDebug() << "could not connect to socket";
        return false;
    } else
    {
        qDebug() << "connection OK";
    }


    // build command and send

    socket.write(makeReadCommand(startAddr, dataSize));
    socket.waitForBytesWritten(100);

    ui->prgSendProgress->setMaximum(dataSize);

    QByteArray recvData;

    while(recvData.size() != dataSize) {
      socket.waitForReadyRead(100);

      recvData.append(socket.readAll());
      ui->prgSendProgress->setValue(recvData.size());
      qDebug() << "received data: " << recvData.size();
    }

    socket.waitForReadyRead(1);
    qDebug() << "read cmd final reply: " << socket.readAll();
    qDebug() << "total received data size: " << recvData.size();

    socket.disconnectFromHost();
    return true;
}
