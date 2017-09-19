// Written by Adrian Musceac YO8RZZ at gmail dot com, started October 2013.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(MumbleClient *client, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _mumble_client(client)
{
    ui->setupUi(this);
    QObject::connect(ui->buttonTransmit,SIGNAL(pressed()),this,SLOT(GUIstartTransmission()));
    //QObject::connect(ui->buttonTransmit,SIGNAL(released()),this,SLOT(GUIendTransmission()));
    QObject::connect(ui->sendTextButton,SIGNAL(clicked()),this,SLOT(GUIsendText()));
    QObject::connect(ui->voipConnectButton,SIGNAL(clicked()),this,SLOT(GUIconnectVOIP()));
    QObject::connect(ui->voipDisconnectButton,SIGNAL(clicked()),this,SLOT(GUIdisconnectVOIP()));
    QObject::connect(ui->voipTalkButton,SIGNAL(pressed()),this,SLOT(GUIstartTalkVOIP()));
    QObject::connect(ui->voipTalkButton,SIGNAL(released()),this,SLOT(GUIstopTalkVOIP()));
    QObject::connect(ui->chooseFileButton,SIGNAL(clicked()),this,SLOT(chooseFile()));
    QObject::connect(ui->rxStatusButton,SIGNAL(toggled(bool)),this,SLOT(toggleRXwin(bool)));
    QObject::connect(ui->txStatusButton,SIGNAL(toggled(bool)),this,SLOT(toggleTXwin(bool)));
    QObject::connect(ui->tuneSlider,SIGNAL(valueChanged(int)),this,SLOT(tuneCenterFreq(int)));
    QObject::connect(ui->frequencyEdit,SIGNAL(returnPressed()),this,SLOT(tuneMainFreq()));
    QObject::connect(ui->txPowerSlider,SIGNAL(valueChanged(int)),this,SLOT(setTxPowerDisplay(int)));
    QObject::connect(ui->modemTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleMode(int)));
    QObject::connect(ui->autotuneButton,SIGNAL(toggled(bool)),this,SLOT(autoTune(bool)));
    ui->tuneSlider->setRange(-100,100);
    _transmitting_radio = false;
    _constellation_gui = ui->widget_const;
    _rssi_gui = ui->widget_rssi;

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::GUIendTransmission()
{
    emit endTransmission();
    ui->redLED->setEnabled(false);
}

void MainWindow::GUIstartTransmission()
{
    if(!_transmitting_radio)
    {
        emit startTransmission();
        ui->redLED->setEnabled(true);
        _transmitting_radio=true;
    }
    else
    {
        _transmitting_radio=false;
        GUIendTransmission();
    }
}

void MainWindow::GUIsendText()
{
    QString text = ui->sendTextEdit->toPlainText();
    emit sendText(text, false);
    ui->sendTextEdit->setPlainText("");
    ui->redLED->setEnabled(true);
}

void MainWindow::displayText(QString text)
{
    ui->receivedTextEdit->setPlainText(ui->receivedTextEdit->toPlainText() + text);
    ui->receivedTextEdit->verticalScrollBar()->setValue(ui->receivedTextEdit->verticalScrollBar()->maximum());
    ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::chooseFile()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "~/", tr("All Files (*.*)"));
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QString filedata = QString(file.readAll().constData());
    emit sendText(filedata, true);
    ui->redLED->setEnabled(true);
}

void MainWindow::displayReceiveStatus(bool status)
{
    ui->greenLED->setEnabled(status);
}

void MainWindow::displayDataReceiveStatus(bool status)
{
    ui->blueLED->setEnabled(status);
}

void MainWindow::displayTransmitStatus(bool status)
{
    ui->redLED->setEnabled(status);
}

void MainWindow::GUIconnectVOIP()
{
    _mumble_client->connectToServer(ui->voipServerEdit->text(), 64738);
    _mumble_client->setMute(false);
}

void MainWindow::GUIdisconnectVOIP()
{
    _mumble_client->disconnectFromServer();
}

void MainWindow::GUIstartTalkVOIP()
{
    emit startTalkVOIP();
}

void MainWindow::GUIstopTalkVOIP()
{
    emit stopTalkVOIP();
}

void MainWindow::updateOnlineStations(StationList stations)
{

}

void MainWindow::toggleRXwin(bool value)
{
    emit toggleRX(value);
}

void MainWindow::toggleTXwin(bool value)
{
    emit toggleTX(value);
}

void MainWindow::toggleWideband(bool value)
{
    emit toggleWidebandMode(value);
}

void MainWindow::toggleMode(int value)
{
    emit toggleModemMode(value);
}

void MainWindow::tuneCenterFreq(int value)
{
    emit fineTuneFreq(ui->tuneSlider->value());
}

void MainWindow::tuneMainFreq()
{
    emit tuneFreq(ui->frequencyEdit->text().toInt());
}

void MainWindow::setTxPowerDisplay(int value)
{
    ui->txPowerDisplay->display(value);
    emit setTxPower(value);
}

void MainWindow::autoTune(bool value)
{
    if(value)
        emit startAutoTuneFreq();
    else
        emit stopAutoTuneFreq();
}

void MainWindow::displayImage(QImage *img)
{
    ui->videoLabel->setPixmap(QPixmap::fromImage(*img,Qt::AutoColor));
}


