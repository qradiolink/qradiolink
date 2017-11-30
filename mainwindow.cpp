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


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    static float tone_list[]= {67.0, 71.9, 74.4, 77.0, 79.7, 82.5, 85.4, 88.5, 81.5, 87.4, 94.8, 100.0, 103.5, 107.2, 110.9,
                         114.8, 118.8, 123.0, 127.3, 131.8, 136.5, 141.3, 146.2, 151.4, 156.7, 162.2,
                          167.9, 173.8, 179.9, 186.2, 192.8, 203.5, 210.7, 218.1, 225.7, 233.6, 241.8, 250.3};
    QStringList tones;
    tones.append("Disabled");
    for(int i=0;i<38;i++)
    {
        tones.append(QString::number(tone_list[i]));
    }

    ui->comboBoxTxCTCSS->addItems(tones);
    ui->comboBoxRxCTCSS->addItems(tones);

    ui->frameCtrlFreq->setup(10, 10U, 9000000000U, 1, UNITS_MHZ );
    ui->frameCtrlFreq->setFrequency(434000000);
    ui->frameCtrlFreq->setBkColor(QColor(0,0,127,255));
    ui->frameCtrlFreq->setHighlightColor(QColor(127,0,0,255));
    ui->frameCtrlFreq->setDigitColor(QColor(230,230,230,240));
    ui->frameCtrlFreq->setUnitsColor(QColor(254,254,254,255));

    QObject::connect(ui->buttonTransmit,SIGNAL(pressed()),this,SLOT(GUIstartTransmission()));
    //QObject::connect(ui->buttonTransmit,SIGNAL(released()),this,SLOT(GUIendTransmission()));
    QObject::connect(ui->sendTextButton,SIGNAL(clicked()),this,SLOT(GUIsendText()));
    QObject::connect(ui->voipConnectButton,SIGNAL(clicked()),this,SLOT(GUIconnectVOIP()));
    QObject::connect(ui->voipDisconnectButton,SIGNAL(clicked()),this,SLOT(GUIdisconnectVOIP()));
    QObject::connect(ui->chooseFileButton,SIGNAL(clicked()),this,SLOT(chooseFile()));
    QObject::connect(ui->clearReceivedTextButton,SIGNAL(clicked()),this,SLOT(clearTextArea()));
    QObject::connect(ui->rxStatusButton,SIGNAL(toggled(bool)),this,SLOT(toggleRXwin(bool)));
    QObject::connect(ui->txStatusButton,SIGNAL(toggled(bool)),this,SLOT(toggleTXwin(bool)));
    QObject::connect(ui->tuneSlider,SIGNAL(valueChanged(int)),this,SLOT(tuneCenterFreq(int)));
    QObject::connect(ui->frequencyEdit,SIGNAL(returnPressed()),this,SLOT(enterFreq()));
    QObject::connect(ui->shiftEdit,SIGNAL(returnPressed()),this,SLOT(enterShift()));
    QObject::connect(ui->txPowerSlider,SIGNAL(valueChanged(int)),this,SLOT(setTxPowerDisplay(int)));
    QObject::connect(ui->rxSensitivitySlider,SIGNAL(valueChanged(int)),this,SLOT(setRxSensitivityDisplay(int)));
    QObject::connect(ui->rxSquelchSlider,SIGNAL(valueChanged(int)),this,SLOT(setSquelchDisplay(int)));
    QObject::connect(ui->rxVolumeSlider,SIGNAL(valueChanged(int)),this,SLOT(setVolumeDisplay(int)));
    QObject::connect(ui->modemTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleMode(int)));
    QObject::connect(ui->autotuneButton,SIGNAL(toggled(bool)),this,SLOT(autoTune(bool)));
    QObject::connect(ui->saveOptionsButton,SIGNAL(clicked()),this,SLOT(saveConfig()));
    QObject::connect(ui->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(mainTabChanged(int)));
    QObject::connect(ui->comboBoxRxCTCSS,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRxCTCSS(int)));
    QObject::connect(ui->comboBoxTxCTCSS,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTxCTCSS(int)));
    QObject::connect(ui->pttVoipButton,SIGNAL(toggled(bool)),this,SLOT(togglePTTVOIP(bool)));
    QObject::connect(ui->voipForwardButton,SIGNAL(toggled(bool)),this,SLOT(toggleVOIPForwarding(bool)));

    QObject::connect(ui->frameCtrlFreq,SIGNAL(newFrequency(qint64)),this,SLOT(tuneMainFreq(qint64)));


    //ui->tuneSlider->setRange(-100,100);
    _transmitting_radio = false;
    _constellation_gui = ui->widget_const;
    _rssi_gui = ui->widget_rssi;
    _fft_gui = ui->widget_fft;
    _config_file = setupConfig();
    readConfig(_config_file);
    _video_img = new QPixmap;
    ui->menuBar->hide();
    setWindowIcon(QIcon(":/res/logo.png"));
    setWindowFlags( Qt::CustomizeWindowHint | Qt::FramelessWindowHint);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    saveConfig();
    emit stopRadio();
    emit disconnectFromServer();
    usleep(200000);
    event->accept();
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
        ui->txPowerSlider->setValue(cfg.lookup("tx_power"));
        ui->rxSensitivitySlider->setValue(cfg.lookup("rx_sensitivity"));
        ui->rxSquelchSlider->setValue(cfg.lookup("squelch"));
        ui->rxVolumeSlider->setValue(cfg.lookup("rx_volume"));
        ui->frameCtrlFreq->setFrequency(cfg.lookup("rx_frequency"));
        _rx_frequency = cfg.lookup("rx_frequency");
        _tx_frequency = cfg.lookup("tx_shift");
        ui->shiftEdit->setText(QString::number(_tx_frequency / 1000));

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
        _rx_frequency = 434000000;
        ui->frameCtrlFreq->setFrequency(_rx_frequency);
        _tx_frequency = 0;
        ui->shiftEdit->setText(QString::number(_tx_frequency,10));

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
    root.add("tx_power",libconfig::Setting::TypeInt) = (int)ui->txPowerSlider->value();
    root.add("rx_sensitivity",libconfig::Setting::TypeInt) = (int)ui->rxSensitivitySlider->value();
    root.add("squelch",libconfig::Setting::TypeInt) = (int)ui->rxSquelchSlider->value();
    root.add("rx_volume",libconfig::Setting::TypeInt) = (int)ui->rxVolumeSlider->value();
    root.add("rx_frequency",libconfig::Setting::TypeInt64) = _rx_frequency;
    root.add("tx_shift",libconfig::Setting::TypeInt64) = _tx_frequency;
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

void MainWindow::clearTextArea()
{
    ui->receivedTextEdit->setPlainText("");
    ui->receivedTextEdit->verticalScrollBar()->setValue(ui->receivedTextEdit->verticalScrollBar()->maximum());
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
    emit connectToServer(ui->voipServerEdit->text(), 64738);
    emit setMute(false);
}

void MainWindow::GUIdisconnectVOIP()
{
    emit disconnectFromServer();
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
    _rx_frequency = freq;
    ui->frequencyEdit->setText(QString::number(ceil(freq/1000)));
    ui->tuneSlider->setValue(0);
    emit tuneFreq(freq);
}

void MainWindow::enterFreq()
{
    ui->frameCtrlFreq->setFrequency(ui->frequencyEdit->text().toLong()*1000);
    _rx_frequency = ui->frequencyEdit->text().toLong()*1000;
    emit tuneFreq(ui->frequencyEdit->text().toLong()*1000);
}

void MainWindow::enterShift()
{
    _tx_frequency = ui->shiftEdit->text().toLong()*1000;
    emit tuneTxFreq(_tx_frequency);
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

void MainWindow::setSquelchDisplay(int value)
{
    ui->rxSquelchDisplay->display(value);
    emit setSquelch(value);
}

void MainWindow::setVolumeDisplay(int value)
{
    ui->rxVolumeDisplay->display(value);
    emit setVolume(value);
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


void MainWindow::mainTabChanged(int value)
{
    if(value == 1)
        emit enableGUIConst(true);
    else
        emit enableGUIConst(false);
    if(value == 2)
    {
        emit enableGUIFFT(true);
    }
    else
    {
        emit enableGUIFFT(false);
    }
}

void MainWindow::updateFreqGUI(long freq)
{
    ui->frameCtrlFreq->setFrequency(freq);
    _rx_frequency = freq;
    ui->frequencyEdit->setText(QString::number(ceil(freq/1000)));
}

void MainWindow::updateRxCTCSS(int value)
{
    emit setRxCTCSS(ui->comboBoxRxCTCSS->currentText().toFloat());
}

void MainWindow::updateTxCTCSS(int value)
{
    emit setTxCTCSS(ui->comboBoxTxCTCSS->currentText().toFloat());
}

void MainWindow::togglePTTVOIP(bool value)
{
    emit usePTTForVOIP(value);
}

void MainWindow::toggleVOIPForwarding(bool value)
{
    emit setVOIPForwarding(value);
}
