#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool writeFromFile(unsigned int startAddr, QString fileName);
    bool readToFile(unsigned int startAddr, unsigned int dataSize, QString fileName);

    void performXMDWriteCmd(QString cmd);

private slots:
    void on_btnSelFile_clicked();

    void on_btnSend_clicked();

    void on_btnReceive_clicked();

    void on_btnExecXMD_clicked();

private:
    Ui::MainWindow *ui;

    QByteArray makeReadCommand(unsigned int addr, unsigned int size);
    QByteArray makeWriteCommand(unsigned int addr, unsigned int size);
};

#endif // MAINWINDOW_H
