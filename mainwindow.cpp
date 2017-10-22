// Written by Adrian Musceac YO8RZZ , started October 2013.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the
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
    ui->frameCtrlFreq->setup(10, 10U, 9000000000U, 1, UNITS_MHZ );
    ui->frameCtrlFreq->setFrequency(433500000);
    ui->frameCtrlFreq->setBkColor(QColor(0,0,127,255));
    ui->frameCtrlFreq->setHighlightColor(QColor(127,0,0,255));
    ui->frameCtrlFreq->setDigitColor(QColor(230,230,230,240));
    ui->frameCtrlFreq->setUnitsColor(QColor(254,254,254,255));

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
    QObject::connect(ui->frequencyEdit,SIGNAL(returnPressed()),this,SLOT(enterFreq()));
    QObject::connect(ui->txPowerSlider,SIGNAL(valueChanged(int)),this,SLOT(setTxPowerDisplay(int)));
    QObject::connect(ui->rxSensitivitySlider,SIGNAL(valueChanged(int)),this,SLOT(setRxSensitivityDisplay(int)));
    QObject::connect(ui->modemTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleMode(int)));
    QObject::connect(ui->autotuneButton,SIGNAL(toggled(bool)),this,SLOT(autoTune(bool)));
    QObject::connect(ui->saveOptionsButton,SIGNAL(clicked()),this,SLOT(saveConfig()));
    QObject::connect(ui->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(mainTabChanged(int)));

    QObject::connect(ui->frameCtrlFreq,SIGNAL(newFrequency(qint64)),this,SLOT(tuneMainFreq(qint64)));

    //ui->tuneSlider->setRange(-100,100);
    _transmitting_radio = false;
    _constellation_gui = ui->widget_const;
    _rssi_gui = ui->widget_rssi;
    _fft_gui = ui->widget_fft;
    _config_file = setupConfig();
    readConfig(_config_file);
    _video_img = new QPixmap;

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readConfig(QFileInfo *config_file)
{
    libconfig::Config cfg;
    try
    {
        cfg.readFile(config_file->absoluteFilePath().toStdString().c_str());
    }
    catch(const libconfig::FileIOException &fioex)
    {
        std::cerr << "I/O error while reading configuration file." << std::endl;
        exit(EXIT_FAILURE);
    }
    catch(const libconfig::ParseException &pex)
    {
        std::cerr << "Configuration parse error at " << pex.getFile() << ":" << pex.getLine()
                  << " - " << pex.getError() << std::endl;
        exit(EXIT_FAILURE);
    }
    try
    {
        int rx_freq_corr, tx_freq_corr;
        cfg.lookupValue("rx_freq_corr", rx_freq_corr);
        cfg.lookupValue("tx_freq_corr", tx_freq_corr);
        ui->lineEditRXDev->setText(QString(cfg.lookup("rx_device_args")));
        ui->lineEditTXDev->setText(QString(cfg.lookup("tx_device_args")));
        ui->lineEditRXAntenna->setText(QString(cfg.lookup("rx_antenna")));
        ui->lineEditTXAntenna->setText(QString(cfg.lookup("tx_antenna")));
        ui->lineEditRXFreqCorrection->setText(QString::number(rx_freq_corr));
        ui->lineEditTXFreqCorrection->setText(QString::number(tx_freq_corr));
        ui->lineEditCallsign->setText(QString(cfg.lookup("callsign")));
        ui->lineEditVideoDevice->setText(QString(cfg.lookup("video_device")));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        ui->lineEditRXDev->setText("rtl=0");
        ui->lineEditTXDev->setText("uhd");
        ui->lineEditRXAntenna->setText("RX2");
        ui->lineEditTXAntenna->setText("TX/RX");
        ui->lineEditRXFreqCorrection->setText("39");
        ui->lineEditTXFreqCorrection->setText("0");
        ui->lineEditCallsign->setText("CALL");
        ui->lineEditVideoDevice->setText("/dev/video0");
        std::cerr << "Settings not found in configuration file." << std::endl;
    }
}

void MainWindow::saveConfig()
{
    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();
    root.add("rx_device_args",libconfig::Setting::TypeString) = ui->lineEditRXDev->text().toStdString();
    root.add("tx_device_args",libconfig::Setting::TypeString) = ui->lineEditTXDev->text().toStdString();
    root.add("rx_antenna",libconfig::Setting::TypeString) = ui->lineEditRXAntenna->text().toStdString();
    root.add("tx_antenna",libconfig::Setting::TypeString) = ui->lineEditTXAntenna->text().toStdString();
    root.add("rx_freq_corr",libconfig::Setting::TypeInt) = ui->lineEditRXFreqCorrection->text().toInt();
    root.add("tx_freq_corr",libconfig::Setting::TypeInt) = ui->lineEditTXFreqCorrection->text().toInt();
    root.add("callsign",libconfig::Setting::TypeString) = ui->lineEditCallsign->text().toStdString();
    root.add("video_device",libconfig::Setting::TypeString) = ui->lineEditVideoDevice->text().toStdString();
    try
    {
        cfg.writeFile(_config_file->absoluteFilePath().toStdString().c_str());
    }
    catch(const libconfig::FileIOException &fioex)
    {
        std::cerr << "I/O error while writing configuration file: " << _config_file->absoluteFilePath().toStdString() << std::endl;
        exit(EXIT_FAILURE);
    }
}

/**
* remove
QFileInfo MainWindow::setupSounds(QString name)
{
    QDir files = QDir::current();

    QFileInfo new_file = files.filePath(name);
    if(!new_file.exists())
    {
        QFile resfile(":/res/" + name);
        QFile newfile(new_file.absoluteFilePath());
        if(resfile.open(QIODevice::ReadOnly))
        {
            if (newfile.open(QIODevice::ReadWrite))
            {
                newfile.write(resfile.readAll());
                newfile.close();
            }
            resfile.close();
        }
    }

    return new_file;
}
*/

QFileInfo* MainWindow::setupConfig()
{
    QDir files = QDir::homePath();

    QFileInfo new_file = files.filePath(".config/qradiolink.cfg");
    if(!new_file.exists())
    {
        QString config = "// Automatically generated\n";
        QFile newfile(new_file.absoluteFilePath());

        if (newfile.open(QIODevice::ReadWrite))
        {
            newfile.write(config.toStdString().c_str());
            newfile.close();
        }

    }

    return new QFileInfo(new_file);
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
    if(ui->receivedTextEdit->toPlainText().size() > 1024*1024*1024)
    {
        // TODO: truncate text
    }
    ui->receivedTextEdit->setPlainText(ui->receivedTextEdit->toPlainText() + text);
    ui->receivedTextEdit->verticalScrollBar()->setValue(ui->receivedTextEdit->verticalScrollBar()->maximum());
    //ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::displayCallsign(QString callsign)
{
    ui->labelDisplayCallsign->setText(callsign);
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
    mainTabChanged(ui->tabWidget->currentIndex());
}

void MainWindow::tuneCenterFreq(int value)
{
    emit fineTuneFreq(ui->tuneSlider->value());
}

void MainWindow::tuneMainFreq(qint64 freq)
{
    ui->frequencyEdit->setText(QString::number(ceil(freq/1000)));
    emit tuneFreq(freq);
}

void MainWindow::enterFreq()
{
    ui->frameCtrlFreq->setFrequency(ui->frequencyEdit->text().toLong()*1000);
    emit tuneFreq(ui->frequencyEdit->text().toLong()*1000);
}

void MainWindow::setTxPowerDisplay(int value)
{
    ui->txPowerDisplay->display(value);
    emit setTxPower(value);
}

void MainWindow::setRxSensitivityDisplay(int value)
{
    ui->rxSensitivityDisplay->display(value);
    emit setRxSensitivity(value);
}

void MainWindow::autoTune(bool value)
{
    if(value)
        emit startAutoTuneFreq();
    else
        emit stopAutoTuneFreq();
}

void MainWindow::displayImage(QImage img)
{
    delete _video_img;
    _video_img = new QPixmap(QPixmap::fromImage(img,Qt::AutoColor));
    ui->videoLabel->setPixmap(*_video_img);
}

void MainWindow::playEndBeep(int seconds)
{

    //_end_beep->play();
}

void MainWindow::mainTabChanged(int value)
{
    if(value == 1)
        emit enableGUI(true);
    else
        emit enableGUI(false);
}

