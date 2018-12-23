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
#include "ui_desktop_mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(Settings *settings, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _settings = settings;
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
    ui->frameCtrlFreq->setFrequency(_settings->rx_frequency);
    ui->frameCtrlFreq->setBkColor(QColor(0,0,99,255));
    ui->frameCtrlFreq->setHighlightColor(QColor(127,0,0,255));
    ui->frameCtrlFreq->setDigitColor(QColor(200,200,200,240));
    ui->frameCtrlFreq->setUnitsColor(QColor(254,254,254,255));

    ui->txGainDial->setStyleSheet("background-color:#660000;");
    ui->rxGainDial->setStyleSheet("background-color:#013E09;");
    ui->rxSquelchDial->setStyleSheet("background-color:#040D55;");
    ui->rxVolumeDial->setStyleSheet("background-color:#040D55;");
    ui->tuneDial->setStyleSheet("background-color:#040D55;");


    QObject::connect(ui->buttonTransmit,SIGNAL(pressed()),this,SLOT(GUIstartTransmission()));
    //QObject::connect(ui->buttonTransmit,SIGNAL(released()),this,SLOT(GUIendTransmission()));
    QObject::connect(ui->sendTextButton,SIGNAL(clicked()),this,SLOT(GUIsendText()));
    QObject::connect(ui->voipConnectButton,SIGNAL(clicked()),this,SLOT(GUIconnectVOIP()));
    QObject::connect(ui->voipDisconnectButton,SIGNAL(clicked()),this,SLOT(GUIdisconnectVOIP()));
    QObject::connect(ui->chooseFileButton,SIGNAL(clicked()),this,SLOT(chooseFile()));
    QObject::connect(ui->clearReceivedTextButton,SIGNAL(clicked()),this,SLOT(clearTextArea()));
    QObject::connect(ui->rxStatusButton,SIGNAL(toggled(bool)),this,SLOT(toggleRXwin(bool)));
    QObject::connect(ui->txStatusButton,SIGNAL(toggled(bool)),this,SLOT(toggleTXwin(bool)));
    QObject::connect(ui->tuneDial,SIGNAL(valueChanged(int)),this,SLOT(tuneCenterFreq(int)));
    QObject::connect(ui->frequencyEdit,SIGNAL(returnPressed()),this,SLOT(enterFreq()));
    QObject::connect(ui->shiftEdit,SIGNAL(returnPressed()),this,SLOT(enterShift()));
    QObject::connect(ui->txGainDial,SIGNAL(valueChanged(int)),this,SLOT(setTxPowerDisplay(int)));
    QObject::connect(ui->rxGainDial,SIGNAL(valueChanged(int)),this,SLOT(setRxSensitivityDisplay(int)));
    QObject::connect(ui->rxSquelchDial,SIGNAL(valueChanged(int)),this,SLOT(setSquelchDisplay(int)));
    QObject::connect(ui->rxVolumeDial,SIGNAL(valueChanged(int)),this,SLOT(setVolumeDisplay(int)));
    QObject::connect(ui->rxModemTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleRxMode(int)));
    QObject::connect(ui->txModemTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleTxMode(int)));
    QObject::connect(ui->autotuneButton,SIGNAL(toggled(bool)),this,SLOT(autoTune(bool)));
    QObject::connect(ui->saveOptionsButton,SIGNAL(clicked()),this,SLOT(saveConfig()));
    QObject::connect(ui->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(mainTabChanged(int)));
    QObject::connect(ui->comboBoxRxCTCSS,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRxCTCSS(int)));
    QObject::connect(ui->comboBoxTxCTCSS,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTxCTCSS(int)));
    QObject::connect(ui->pttVoipButton,SIGNAL(toggled(bool)),this,SLOT(togglePTTVOIP(bool)));
    QObject::connect(ui->voipForwardButton,SIGNAL(toggled(bool)),this,SLOT(toggleVOIPForwarding(bool)));
    QObject::connect(ui->toggleRepeaterButton,SIGNAL(toggled(bool)),this,SLOT(toggleRepeater(bool)));
    QObject::connect(ui->toggleVoxButton,SIGNAL(toggled(bool)),this,SLOT(toggleVox(bool)));

    QObject::connect(ui->frameCtrlFreq,SIGNAL(newFrequency(qint64)),this,SLOT(tuneMainFreq(qint64)));

    QObject::connect(ui->voipTreeWidget,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT(channelState(QTreeWidgetItem *,int)));

    ui->rxModemTypeComboBox->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->txModemTypeComboBox->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->sendTextEdit->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->receivedTextEdit->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->voipTreeWidget->setColumnHidden(2,true);
    ui->voipTreeWidget->setColumnHidden(3,true);
    _transmitting_radio = false;
    _current_voip_channel = -1;
    _constellation_gui = ui->widget_const;
    _rssi_gui = ui->widget_rssi;
    _fft_gui = ui->widget_fft;
    readConfig();
    _video_img = new QPixmap;
    ui->menuBar->hide();
    ui->statusBar->hide();
    ui->mainToolBar->hide();
    setWindowIcon(QIcon(":/res/logo.png"));


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
    struct timespec time_to_sleep = {0, 200000000L };
    nanosleep(&time_to_sleep, NULL);
    event->accept();
}

void MainWindow::readConfig()
{
    ui->lineEditRXDev->setText(_settings->rx_device_args);
    ui->lineEditTXDev->setText(_settings->tx_device_args);
    ui->lineEditRXAntenna->setText(_settings->rx_antenna);
    ui->lineEditTXAntenna->setText(_settings->tx_antenna);
    ui->lineEditRXFreqCorrection->setText(QString::number(_settings->rx_freq_corr));
    ui->lineEditTXFreqCorrection->setText(QString::number(_settings->tx_freq_corr));
    ui->lineEditCallsign->setText(_settings->callsign);
    ui->lineEditVideoDevice->setText(_settings->video_device);
    ui->txGainDial->setValue(_settings->tx_power);
    ui->lineEditBBgain->setText(QString::number(_settings->bb_gain));
    ui->rxGainDial->setValue(_settings->rx_sensitivity);
    ui->rxSquelchDial->setValue(_settings->squelch);
    ui->rxVolumeDial->setValue(_settings->rx_volume);
    ui->frameCtrlFreq->setFrequency(_settings->rx_frequency);
    _rx_frequency = _settings->rx_frequency;
    ui->frequencyEdit->setText(QString::number(ceil(_rx_frequency/1000)));
    _tx_frequency = _settings->tx_shift;
    ui->shiftEdit->setText(QString::number(_tx_frequency / 1000));
    ui->voipServerEdit->setText(_settings->voip_server);
    ui->rxModemTypeComboBox->setCurrentIndex(_settings->rx_mode);
    ui->txModemTypeComboBox->setCurrentIndex(_settings->tx_mode);
    ui->lineEditIPaddress->setText(_settings->ip_address);

}

void MainWindow::saveConfig()
{
    _settings->rx_device_args = ui->lineEditRXDev->text();
    _settings->tx_device_args = ui->lineEditTXDev->text();
    _settings->rx_antenna = ui->lineEditRXAntenna->text();
    _settings->tx_antenna = ui->lineEditTXAntenna->text();
    _settings->rx_freq_corr = ui->lineEditRXFreqCorrection->text().toInt();
    _settings->tx_freq_corr = ui->lineEditTXFreqCorrection->text().toInt();
    _settings->callsign = ui->lineEditCallsign->text();
    _settings->video_device = ui->lineEditVideoDevice->text();
    _settings->tx_power = (int)ui->txGainDial->value();
    _settings->bb_gain = (int)ui->lineEditBBgain->text().toInt();
    _settings->rx_sensitivity = (int)ui->rxGainDial->value();
    _settings->squelch = (int)ui->rxSquelchDial->value();
    _settings->rx_volume = (int)ui->rxVolumeDial->value();
    _settings->rx_frequency = _rx_frequency;
    _settings->tx_shift = _tx_frequency;
    _settings->voip_server = ui->voipServerEdit->text();
    _settings->rx_mode = ui->rxModemTypeComboBox->currentIndex();
    _settings->tx_mode = ui->txModemTypeComboBox->currentIndex();
    _settings->ip_address = ui->lineEditIPaddress->text();
    _settings->saveConfig();
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

void MainWindow::displayText(QString text, bool html)
{
    if(ui->receivedTextEdit->toPlainText().size() > 1024*1024*1024)
    {
        // TODO: truncate text
    }
    if(html)
        ui->receivedTextEdit->insertHtml(text);
    else
        ui->receivedTextEdit->insertPlainText(text);
    ui->receivedTextEdit->verticalScrollBar()->setValue(ui->receivedTextEdit->verticalScrollBar()->maximum());
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
    ui->voipTreeWidget->clear();
}


void MainWindow::updateOnlineStations(StationList stations)
{
    for(int i=0;i<stations.size();i++)
    {
        QList<QTreeWidgetItem*> list = ui->voipTreeWidget->findItems(".", Qt::MatchRegExp | Qt::MatchExactly | Qt::MatchRecursive,3);
        if(list.size()>0)
        {
            delete list.at(0);
        }
    }
    for(int i=0;i<stations.size();i++)
    {
        QList<QTreeWidgetItem*> channel_list = ui->voipTreeWidget->findItems(QString::number(stations.at(i).channel_id),
                                                                             Qt::MatchExactly | Qt::MatchRecursive,2);
        if(channel_list.size()>0)
        {
            QTreeWidgetItem *item = channel_list.at(0);
            QTreeWidgetItem *st_item = new QTreeWidgetItem(0);
            st_item->setText(0,stations.at(i).callsign);
            st_item->setText(3,QString::number(stations.at(i).id));
            item->addChild(st_item);
        }
    }
}

void MainWindow::newChannel(Channel *chan)
{
    if(chan->name.isEmpty())
    {
        QList<QTreeWidgetItem*> channel_list = ui->voipTreeWidget->findItems(QString::number(_current_voip_channel),Qt::MatchExactly | Qt::MatchRecursive,2);
        if(channel_list.size() > 0)
        {
            QTreeWidgetItem *old_item = channel_list.at(0);
            old_item->setBackgroundColor(0,QColor("#ffffff"));
            old_item->setBackgroundColor(1,QColor("#ffffff"));
            old_item->setBackgroundColor(2,QColor("#ffffff"));
            old_item->setTextColor(0,QColor("#000000"));
            old_item->setTextColor(1,QColor("#000000"));
            old_item->setTextColor(2,QColor("#000000"));
        }
        QTreeWidgetItem *item = ui->voipTreeWidget->findItems(QString::number(chan->id),Qt::MatchExactly | Qt::MatchRecursive,2).at(0);
        item->setBackgroundColor(0,QColor("#770000"));
        item->setBackgroundColor(1,QColor("#770000"));
        item->setBackgroundColor(2,QColor("#770000"));
        item->setTextColor(0,QColor("#ffffff"));
        item->setTextColor(1,QColor("#ffffff"));
        item->setTextColor(2,QColor("#ffffff"));
        _current_voip_channel = chan->id;
        ui->voipTreeWidget->expandAll();
        return;
    }
    QTreeWidgetItem *t = new QTreeWidgetItem(0);
    t->setText(2,QString::number(chan->id));
    t->setText(0,chan->name);
    t->setText(1,chan->description);
    t->setBackgroundColor(0,QColor("#ffffff"));
    t->setBackgroundColor(1,QColor("#ffffff"));
    t->setBackgroundColor(2,QColor("#ffffff"));
    t->setTextColor(0,QColor("#000000"));
    t->setTextColor(1,QColor("#000000"));
    t->setTextColor(2,QColor("#000000"));
    if(chan->parent_id <= 0)
        ui->voipTreeWidget->addTopLevelItem(t);
    else
    {
        QList<QTreeWidgetItem*> channel_list = ui->voipTreeWidget->findItems(QString::number(chan->parent_id),Qt::MatchExactly | Qt::MatchRecursive,2);
        if(channel_list.size() > 0)
        {
            QTreeWidgetItem *parent = channel_list.at(0);
            parent->addChild(t);
            t->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
    }
    ui->voipTreeWidget->expandAll();
}

void MainWindow::channelState(QTreeWidgetItem *item, int k)
{
    Q_UNUSED(k);
    emit changeChannel((int)item->data(2,0).toInt());
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

void MainWindow::toggleRxMode(int value)
{
    emit toggleRxModemMode(value);
    mainTabChanged(ui->tabWidget->currentIndex());
}

void MainWindow::toggleTxMode(int value)
{
    emit toggleTxModemMode(value);
    mainTabChanged(ui->tabWidget->currentIndex());
}

void MainWindow::toggleRepeater(bool value)
{
    emit toggleRepeat(value);
}

void MainWindow::tuneCenterFreq(int value)
{
    emit fineTuneFreq((int)ui->tuneDial->value());
}

void MainWindow::tuneMainFreq(qint64 freq)
{
    _rx_frequency = freq;
    ui->frequencyEdit->setText(QString::number(ceil(freq/1000)));
    ui->tuneDial->setValue(0);
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
    emit setTxPower((int)value);
}

void MainWindow::setRxSensitivityDisplay(int value)
{
    ui->rxSensitivityDisplay->display(value);
    emit setRxSensitivity((int)value);
}

void MainWindow::setSquelchDisplay(int value)
{
    ui->rxSquelchDisplay->display(value);
    emit setSquelch((int)value);
}

void MainWindow::setVolumeDisplay(int value)
{
    ui->rxVolumeDisplay->display(value);
    emit setVolume((int)value);
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
    ui->videoLabel->clear();
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

void MainWindow::toggleVox(bool value)
{
    emit setVox(value);
}
