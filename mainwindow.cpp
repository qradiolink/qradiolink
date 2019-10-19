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


MainWindow::MainWindow(Settings *settings, RadioChannels *radio_channels, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    _settings = settings;
    _radio_channels = radio_channels;

    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-Normal.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-Bold.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-BoldItalic.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-NormalItalic.otf");
    setAnimated(true);
    ui->setupUi(this);

    QStringList tones;
    tones.append("CTCSS");
    for(int i=0;i<38;i++)
    {
        tones.append(QString::number(tone_list[i]));
    }
    ui->comboBoxTxCTCSS->addItems(tones);
    ui->comboBoxRxCTCSS->addItems(tones);

    _filter_widths = new std::vector<std::complex<int>>;
    _filter_ranges = new std::vector<std::complex<int>>;
    _filter_symmetric = new std::vector<bool>;

    buildFilterWidthList(_filter_widths, _filter_ranges, _filter_symmetric);

    _audio_output_devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    _audio_input_devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i = 0;i<_audio_input_devices.size();i++)
    {
        ui->audioInputComboBox->addItem(_audio_input_devices.at(i).deviceName());
    }
    for(int i = 0;i<_audio_output_devices.size();i++)
    {
        ui->audioOutputComboBox->addItem(_audio_output_devices.at(i).deviceName());
    }


    ui->tabWidget->setCurrentIndex(0);

    ui->frameCtrlFreq->setup(10, 10U, 9000000000U, 1, UNITS_MHZ );
    //ui->frameCtrlFreq->setBkColor(QColor(202, 194, 197,0xFF));
    ui->frameCtrlFreq->setBkColor(QColor(0x1F, 0x1D, 0x1D,0xFF));
    ui->frameCtrlFreq->setHighlightColor(QColor(127,55,55,0xFF));
    //ui->frameCtrlFreq->setDigitColor(QColor(0,0,133,0xFF));
    ui->frameCtrlFreq->setDigitColor(QColor(0,205,0,0xFF));
    ui->frameCtrlFreq->setUnitsColor(QColor(254,254,254,0xFF));


    ui->txGainDial->setStyleSheet("background-color: rgb(150, 150, 150);");
    ui->rxGainDial->setStyleSheet("background-color: rgb(150, 150, 150);");
    ui->rxSquelchDial->setStyleSheet("background-color:rgb(150, 150, 150);");
    ui->rxVolumeDial->setStyleSheet("background-color: rgb(150, 150, 150);");
    ui->tuneDial->setStyleSheet("background-color: rgb(150, 150, 150);");
    ui->txGainDial->setNotchesVisible(true);


    QObject::connect(ui->buttonTransmit,SIGNAL(toggled(bool)),this,SLOT(startTx()));
    QObject::connect(ui->sendTextButton,SIGNAL(clicked()),this,SLOT(sendTextRequested()));
    QObject::connect(ui->voipConnectButton,SIGNAL(clicked()),this,SLOT(connectVOIPRequested()));
    QObject::connect(ui->voipDisconnectButton,SIGNAL(clicked()),this,SLOT(disconnectVOIPRequested()));
    QObject::connect(ui->chooseFileButton,SIGNAL(clicked()),this,SLOT(chooseFile()));
    QObject::connect(ui->clearReceivedTextButton,SIGNAL(clicked()),this,SLOT(clearTextArea()));
    QObject::connect(ui->rxStatusButton,SIGNAL(toggled(bool)),this,SLOT(toggleRXwin(bool)));
    QObject::connect(ui->txStatusButton,SIGNAL(toggled(bool)),this,SLOT(toggleTXwin(bool)));
    QObject::connect(ui->tuneDial,SIGNAL(valueChanged(int)),this,SLOT(clarifierTuneFreq(int)));
    QObject::connect(ui->frequencyEdit,SIGNAL(returnPressed()),this,SLOT(enterFreq()));
    QObject::connect(ui->shiftEdit,SIGNAL(returnPressed()),this,SLOT(enterShift()));
    QObject::connect(ui->txGainDial,SIGNAL(valueChanged(int)),this,SLOT(setTxPowerDisplay(int)));
    QObject::connect(ui->rxGainDial,SIGNAL(valueChanged(int)),this,SLOT(setRxSensitivityDisplay(int)));
    QObject::connect(ui->rxSquelchDial,SIGNAL(valueChanged(int)),this,SLOT(setSquelchDisplay(int)));
    QObject::connect(ui->autoSquelchButton,SIGNAL(clicked()),this,SLOT(autoSquelch()));
    QObject::connect(ui->memoriesButton,SIGNAL(toggled(bool)),this,SLOT(showMemoriesPanel(bool)));
    QObject::connect(ui->rxVolumeDial,SIGNAL(valueChanged(int)),this,SLOT(setVolumeDisplay(int)));
    QObject::connect(ui->micGainSlider,SIGNAL(valueChanged(int)),this,SLOT(setTxVolumeDisplay(int)));
    QObject::connect(ui->digitalGainSlider,SIGNAL(valueChanged(int)),this,SLOT(setDigitalGain(int)));
    QObject::connect(ui->voipGainSlider,SIGNAL(valueChanged(int)),this,SLOT(changeVoipVolume(int)));
    QObject::connect(ui->rxModemTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleRxMode(int)));
    QObject::connect(ui->txModemTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleTxMode(int)));
    QObject::connect(ui->scanUpButton,SIGNAL(toggled(bool)),this,SLOT(startScan(bool)));
    QObject::connect(ui->scanDownButton,SIGNAL(toggled(bool)),this,SLOT(startScan(bool)));
    QObject::connect(ui->memoryScanUpButton,SIGNAL(toggled(bool)),this,SLOT(startMemoryScan(bool)));
    QObject::connect(ui->memoryScanDownButton,SIGNAL(toggled(bool)),this,SLOT(startMemoryScan(bool)));
    QObject::connect(ui->saveOptionsButton,SIGNAL(clicked()),this,SLOT(saveUiConfig()));
    QObject::connect(ui->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(mainTabChanged(int)));
    QObject::connect(ui->comboBoxRxCTCSS,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRxCTCSS(int)));
    QObject::connect(ui->comboBoxTxCTCSS,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTxCTCSS(int)));
    QObject::connect(ui->pttVoipButton,SIGNAL(toggled(bool)),this,SLOT(togglePTTVOIP(bool)));
    QObject::connect(ui->voipForwardButton,SIGNAL(toggled(bool)),this,SLOT(toggleVOIPForwarding(bool)));
    QObject::connect(ui->toggleRepeaterButton,SIGNAL(toggled(bool)),this,SLOT(toggleRepeater(bool)));
    QObject::connect(ui->toggleVoxButton,SIGNAL(toggled(bool)),this,SLOT(toggleVox(bool)));
    QObject::connect(ui->fftSizeBox,SIGNAL(currentIndexChanged(int)),this,SLOT(setFFTSize(int)));
    QObject::connect(ui->peakHoldCheckBox,SIGNAL(toggled(bool)),ui->plotterFrame,SLOT(setPeakHold(bool)));
    QObject::connect(ui->showControlsButton,SIGNAL(toggled(bool)),this,SLOT(showControls(bool)));
    QObject::connect(ui->showConstellationButton,SIGNAL(toggled(bool)),this,SLOT(showConstellation(bool)));
    QObject::connect(ui->duplexOpButton,SIGNAL(toggled(bool)),this,SLOT(setEnabledDuplex(bool)));
    QObject::connect(ui->fftEnableCheckBox,SIGNAL(toggled(bool)),this,SLOT(setEnabledFFT(bool)));
    QObject::connect(ui->peakDetectCheckBox,SIGNAL(toggled(bool)),this,SLOT(setPeakDetect(bool)));
    QObject::connect(ui->fpsBox,SIGNAL(currentIndexChanged(int)),this,SLOT(newWaterfallFPS()));
    QObject::connect(ui->sampleRateBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateSampleRate()));
    QObject::connect(ui->checkBoxAudioCompressor,SIGNAL(toggled(bool)),this,SLOT(setAudioCompressor(bool)));
    QObject::connect(ui->checkBoxRelays,SIGNAL(toggled(bool)),this,SLOT(setRelays(bool)));
    QObject::connect(ui->remoteControlCheckBox,SIGNAL(toggled(bool)),this,SLOT(setRemoteControl(bool)));
    QObject::connect(ui->rssiCalibrateButton,SIGNAL(clicked()),this,SLOT(setRSSICalibration()));
    QObject::connect(ui->saveChannelsButton,SIGNAL(clicked()),this,SLOT(saveMemoryChannes()));
    QObject::connect(ui->agcAttackSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateAgcAttack(int)));
    QObject::connect(ui->agcDecaySpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateAgcDecay(int)));
    QObject::connect(ui->mumbleTextMessageButton,SIGNAL(clicked()),this,SLOT(sendMumbleTextMessage()));
    QObject::connect(ui->mumbleTextMessageEdit,SIGNAL(returnPressed()),this,SLOT(sendMumbleTextMessage()));
    QObject::connect(ui->muteSelfButton,SIGNAL(toggled(bool)),this,SLOT(toggleSelfMute(bool)));
    QObject::connect(ui->deafenSelfButton,SIGNAL(toggled(bool)),this,SLOT(toggleSelfDeaf(bool)));
    QObject::connect(ui->voipGainSlider,SIGNAL(valueChanged(int)),this,SLOT(changeVoipVolume(int)));


    QObject::connect(ui->frameCtrlFreq,SIGNAL(newFrequency(qint64)),this,SLOT(tuneMainFreq(qint64)));
    QObject::connect(ui->plotterFrame,SIGNAL(pandapterRangeChanged(float,float)),ui->plotterFrame,SLOT(setWaterfallRange(float,float)));
    QObject::connect(ui->plotterFrame,SIGNAL(newCenterFreq(qint64)),this,SLOT(tuneFreqPlotter(qint64)));
    QObject::connect(ui->plotterFrame,SIGNAL(newFilterFreq(int, int)),this,SLOT(changeFilterWidth(int, int)));
    QObject::connect(ui->panadapterSlider,SIGNAL(valueChanged(int)),ui->plotterFrame,SLOT(setPercent2DScreen(int)));
    QObject::connect(ui->averagingSlider,SIGNAL(valueChanged(int)),this,SLOT(setAveraging(int)));
    QObject::connect(ui->rangeSlider,SIGNAL(valueChanged(int)),this,SLOT(setFFTRange(int)));
    QObject::connect(ui->plotterFrame,SIGNAL(newDemodFreq(qint64,qint64)),this,SLOT(carrierOffsetChanged(qint64,qint64)));

    QObject::connect(ui->voipTreeWidget,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT(channelState(QTreeWidgetItem *,int)));
    QObject::connect(ui->memoriesTableWidget,SIGNAL(cellClicked(int, int)),this,SLOT(tuneToMemoryChannel(int, int)));
    QObject::connect(ui->addChannelButton,SIGNAL(clicked()), this, SLOT(addMemoryChannel()));
    QObject::connect(ui->removeChannelButton,SIGNAL(clicked()), this, SLOT(removeMemoryChannel()));
    QObject::connect(ui->memoriesTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(editMemoryChannel(QTableWidgetItem*)));

    QObject::connect(&_secondary_text_timer,SIGNAL(timeout()),ui->secondaryTextDisplay,SLOT(hide()));
    QObject::connect(&_video_timer,SIGNAL(timeout()),ui->videoFrame,SLOT(hide()));
    QObject::connect(&_speech_icon_timer,SIGNAL(timeout()),this,SLOT(resetSpeechIcons()));

    ui->rxModemTypeComboBox->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->txModemTypeComboBox->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->sendTextEdit->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->receivedTextEdit->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->voipTreeWidget->setColumnHidden(2,true);
    ui->voipTreeWidget->setColumnHidden(3,true);

    ui->controlsFrame->hide();
    ui->constellationDisplay->hide();
    ui->secondaryTextDisplay->hide();
    ui->videoFrame->hide();
    ui->menuBar->hide();
    ui->statusBar->hide();
    ui->mainToolBar->hide();
    ui->memoriesFrame->hide();

    _video_img = new QPixmap;
    _constellation_img = new QPixmap(300,300);
    _realFftData = new float[1024*1024];
    _iirFftData = new float[1024*1024];
    _s_meter_bg = new QPixmap(":/res/s-meter-bg-black-small.png");
    _current_voip_channel = -1;

    _rssi = 0;
    QRect xy = this->geometry();
    ui->plotterContainer->resize(xy.right() -xy.left()-20,xy.bottom()-xy.top()-120);
    ui->secondaryTextDisplay->move(xy.left() + 5, xy.bottom() - 265);
    ui->plotterFrame->setSampleRate(1000000);
    ui->plotterFrame->setSpanFreq((quint32)1000000);
    ui->plotterFrame->setRunningState(false);
    //ui->plotterFrame->setFftRate(10);
    ui->plotterFrame->setPercent2DScreen(50);
    ui->plotterFrame->setFftFill(true);
    ui->plotterFrame->setFreqDigits(3);
    ui->plotterFrame->setTooltipsEnabled(true);
    ui->plotterFrame->setClickResolution(1);
    ui->plotterFrame->setFftRange(-120.0,-30.0);
    ui->plotterFrame->setWaterfallRange(-120.0,-30.0);
    //setFFTRange(1);
    _range_set = false;
    //QPixmap pm = QPixmap::grabWidget(ui->frameCtrlFreq);
    //ui->frameCtrlFreq->setMask(pm.createHeuristicMask(false));
    _eff_freq = new QGraphicsOpacityEffect(this);
    _eff_freq->setOpacity(0.6);
    ui->frameCtrlFreq->setGraphicsEffect(_eff_freq);
    _eff_const = new QGraphicsOpacityEffect(this);
    _eff_const->setOpacity(0.6);
    ui->constellationDisplay->setGraphicsEffect(_eff_const);
    _constellation_painter = new QPainter(_constellation_img);
    _constellation_painter->end();
    _eff_video = new QGraphicsOpacityEffect(this);
    _eff_video->setOpacity(0.95);
    ui->videoFrame->setGraphicsEffect(_eff_video);
    _eff_text_display = new QGraphicsOpacityEffect(this);
    _eff_text_display->setOpacity(0.5);
    ui->secondaryTextDisplay->setGraphicsEffect(_eff_text_display);
    _eff_mem_display = new QGraphicsOpacityEffect(ui->memoriesFrame);
    _eff_mem_display->setOpacity(0.8);
    ui->memoriesFrame->setGraphicsEffect(_eff_mem_display);
    ui->memoryControlsFrame->setGraphicsEffect(_eff_mem_display);
    ui->memoriesTableWidget->setGraphicsEffect(_eff_mem_display);

    _speech_icon_timer.setSingleShot(true);
    _secondary_text_timer.setSingleShot(true);

    setWindowIcon(QIcon(":/res/logo.png"));
    setWindowTitle("QRadioLink");
}

void MainWindow::initSettings()
{
    setConfig();
    updateMemories();
    updateRSSI(9999);
    _range_set = false;
    //setFFTRange(1);
    setEnabledFFT((bool)_settings->show_fft);
    setEnabledDuplex((bool) _settings->enable_duplex);
    setAudioCompressor((bool) _settings->audio_compressor);
    _range_set = false;
    ui->showConstellationButton->setChecked(_settings->show_constellation);
    showConstellation(_settings->show_constellation);
    ui->showControlsButton->setChecked((bool)_settings->show_controls);
    showControls((bool)_settings->show_controls);
    toggleRxMode(_settings->rx_mode);
    toggleTxMode(_settings->tx_mode);
    setTxVolumeDisplay(_settings->tx_volume);
    changeVoipVolume(_settings->voip_volume);
    setRelays((bool)_settings->enable_relays);
    setRSSICalibration();
}

MainWindow::~MainWindow()
{
    for(int i =0;i<_rx_gain_sliders.size();i++)
    {
        delete _rx_gain_sliders.at(i);
    }
    for(int i =0;i<_tx_gain_sliders.size();i++)
    {
        delete _tx_gain_sliders.at(i);
    }
    _rx_gain_sliders.clear();
    _tx_gain_sliders.clear();
    _filter_widths->clear();
    delete _filter_widths;
    _filter_ranges->clear();
    delete _filter_ranges;
    _filter_symmetric->clear();
    delete _filter_symmetric;
    delete _video_img;
    if(_constellation_painter->isActive())
        _constellation_painter->end();
    delete _constellation_img;
    delete _constellation_painter;
    delete[] _realFftData;
    delete[] _iirFftData;
    delete _eff_freq;
    delete _eff_const;
    delete _eff_video;
    delete _eff_text_display;
    delete ui;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    saveUiConfig();
    emit stopRadio();
    emit disconnectFromServer();
    emit terminateConnections();
    // FIXME: this is the wrong way to stop the radio
    struct timespec time_to_sleep = {0, 200000000L };
    nanosleep(&time_to_sleep, NULL);
    event->accept();
}


void MainWindow::resizeEvent(QResizeEvent *event)
{
    QRect xy = this->geometry();
    if((bool)_settings->show_controls)
    {
        ui->plotterContainer->resize(xy.right() -xy.left()-20,xy.bottom()-xy.top()-210);
        xy = ui->plotterContainer->geometry();
        ui->secondaryTextDisplay->move(xy.left(), xy.bottom() - 150);
        ui->memoriesFrame->move(xy.right() - 30 - 900, xy.bottom() - 285);
    }
    else
    {
        ui->plotterContainer->resize(xy.right() -xy.left()-20,xy.bottom()-xy.top()-120);
        xy = ui->plotterContainer->geometry();
        ui->secondaryTextDisplay->move(xy.left(), xy.bottom() - 150);
        ui->memoriesFrame->move(xy.right() - 30 - 900, xy.bottom() - 285);
    }
    xy = ui->plotterContainer->geometry();
    ui->videoFrame->move(xy.right() - 360, xy.top());
    event->accept();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        /** Disabled! Due to logic changes in radioop, this breaks
        if (isMinimized())
        {
            emit enableGUIFFT(false);
            emit enableGUIConst(false);
            emit enableRSSI(false);
        }
        else
        {
            emit enableGUIFFT(true);
            emit enableGUIConst(true);
            emit enableRSSI(true);
        }
        */
    }

    event->accept();
}

void MainWindow::showControls(bool value)
{
    QRect xy = this->geometry();
    if(value)
    {
        ui->plotterContainer->resize(xy.right() -xy.left()-20,xy.bottom()-xy.top()-210);
        ui->controlsFrame->show();
        xy = ui->plotterContainer->geometry();
        ui->secondaryTextDisplay->move(xy.left(), xy.bottom() - 150);
        ui->memoriesFrame->move(xy.right() - 30 - 900, xy.bottom() - 285);
        _settings->show_controls = 1;
    }
    else
    {
        ui->plotterContainer->resize(xy.right() -xy.left()-20,xy.bottom()-xy.top()-120);
        ui->controlsFrame->hide();
        xy = ui->plotterContainer->geometry();
        ui->secondaryTextDisplay->move(xy.left(), xy.bottom() - 150);
        ui->memoriesFrame->move(xy.right() - 30 - 900, xy.bottom() - 285);
        _settings->show_controls = 0;
    }
    emit enableRSSI(value);
}

void MainWindow::showConstellation(bool value)
{
    _settings->show_constellation = (int) value;
    if(value)
    {
        ui->constellationDisplay->show();
    }
    else
    {
        ui->constellationDisplay->hide();
    }
    emit enableGUIConst(value);
}

void MainWindow::showMemoriesPanel(bool value)
{
    if(value)
    {
        ui->memoriesFrame->show();
        ui->memoriesFrame->activateWindow();
        ui->memoriesFrame->raise();
    }
    else
    {
        ui->memoriesFrame->hide();
    }
}

void MainWindow::setEnabledFFT(bool value)
{
    _settings->show_fft = (int) value;
    emit enableGUIFFT(value);
}

void MainWindow::setEnabledDuplex(bool value)
{
    _settings->enable_duplex = (int) value;
    emit enableDuplex(value);
}

void MainWindow::setConfig()
{
    ui->lineEditRXDev->setText(_settings->rx_device_args);
    ui->lineEditTXDev->setText(_settings->tx_device_args);
    ui->lineEditRXAntenna->setText(_settings->rx_antenna);
    ui->lineEditTXAntenna->setText(_settings->tx_antenna);
    ui->lineEditRXFreqCorrection->setText(QString::number(_settings->rx_freq_corr));
    ui->lineEditTXFreqCorrection->setText(QString::number(_settings->tx_freq_corr));
    ui->lineEditCallsign->setText(_settings->callsign);
    ui->lineEditVideoDevice->setText(_settings->video_device);
    ui->audioInputComboBox->setCurrentText(_settings->audio_input_device);
    ui->audioOutputComboBox->setCurrentText(_settings->audio_output_device);
    ui->txGainDial->setValue(_settings->tx_power);
    ui->digitalGainSlider->setValue(_settings->bb_gain);
    ui->rxGainDial->setValue(_settings->rx_sensitivity);
    ui->rxSquelchDial->setValue(_settings->squelch);
    ui->rxVolumeDial->setValue(_settings->rx_volume);
    ui->micGainSlider->setValue(_settings->tx_volume);
    ui->voipServerPortEdit->setText(QString::number(_settings->voip_port));
    ui->voipPasswordEdit->setText(_settings->voip_password);
    ui->remoteControlEdit->setText(QString::number(_settings->control_port));

    ui->frequencyEdit->setText(QString::number(ceil(_settings->rx_frequency/1000)));
    ui->shiftEdit->setText(QString::number(_settings->tx_shift / 1000));
    ui->voipServerEdit->setText(_settings->voip_server);
    ui->rxModemTypeComboBox->setCurrentIndex(_settings->rx_mode);
    ui->txModemTypeComboBox->setCurrentIndex(_settings->tx_mode);
    ui->lineEditIPaddress->setText(_settings->ip_address);
    ui->plotterFrame->setFilterOffset((qint64)_settings->demod_offset);
    ui->plotterFrame->setCenterFreq(_settings->rx_frequency);
    ui->frameCtrlFreq->setFrequency(_settings->rx_frequency + _settings->demod_offset);
    ui->plotterFrame->setSampleRate(_settings->rx_sample_rate);
    ui->plotterFrame->setSpanFreq((quint32)_settings->rx_sample_rate);
    ui->sampleRateBox->setCurrentIndex(ui->sampleRateBox->findText(QString::number(_settings->rx_sample_rate)));
    ui->averagingSlider->setValue((int)(1.0f/_settings->fft_averaging));
    ui->fftSizeBox->setCurrentIndex(ui->fftSizeBox->findText(QString::number(_settings->fft_size)));
    ui->fpsBox->setCurrentIndex(ui->fpsBox->findText(QString::number(_settings->waterfall_fps)));
    ui->lineEditScanStep->setText(QString::number(_settings->scan_step));
    ui->fftEnableCheckBox->setChecked((bool)_settings->show_fft);
    ui->duplexOpButton->setChecked((bool) _settings->enable_duplex);
    ui->checkBoxAudioCompressor->setChecked((bool)_settings->audio_compressor);
    ui->checkBoxRelays->setChecked((bool)_settings->enable_relays);
    ui->remoteControlCheckBox->setChecked((bool)_settings->remote_control);
    ui->rssiCalibrateEdit->setText(QString::number(_settings->rssi_calibration_value));
    if(_settings->rx_ctcss > 0.0)
        ui->comboBoxRxCTCSS->setCurrentText(QString::number(_settings->rx_ctcss));
    else
        ui->comboBoxRxCTCSS->setCurrentText("CTCSS");
    if(_settings->tx_ctcss > 0.0)
        ui->comboBoxTxCTCSS->setCurrentText(QString::number(_settings->tx_ctcss));
    else
        ui->comboBoxTxCTCSS->setCurrentText("CTCSS");
    ui->agcAttackSpinBox->setValue(_settings->agc_attack);
    ui->agcDecaySpinBox->setValue(_settings->agc_decay);

}

void MainWindow::saveUiConfig()
{
    _settings->rx_device_args = ui->lineEditRXDev->text();
    _settings->tx_device_args = ui->lineEditTXDev->text();
    _settings->rx_antenna = ui->lineEditRXAntenna->text();
    _settings->tx_antenna = ui->lineEditTXAntenna->text();
    _settings->rx_freq_corr = ui->lineEditRXFreqCorrection->text().toInt();
    _settings->tx_freq_corr = ui->lineEditTXFreqCorrection->text().toInt();    
    _settings->callsign = ui->lineEditCallsign->text();
    _settings->video_device = ui->lineEditVideoDevice->text();
    _settings->audio_input_device = ui->audioInputComboBox->currentText();
    _settings->audio_output_device = ui->audioOutputComboBox->currentText();
    _settings->tx_power = (int)ui->txGainDial->value();
    _settings->bb_gain = (int)ui->digitalGainSlider->value();
    _settings->rx_sensitivity = (int)ui->rxGainDial->value();
    _settings->squelch = (int)ui->rxSquelchDial->value();
    _settings->rx_volume = (int)ui->rxVolumeDial->value();
    _settings->tx_volume = (int)ui->micGainSlider->value();
    _settings->voip_server = ui->voipServerEdit->text();
    _settings->voip_port = ui->voipServerPortEdit->text().toInt();
    _settings->voip_password = ui->voipPasswordEdit->text();
    _settings->control_port = ui->remoteControlEdit->text().toInt();
    _settings->rx_mode = ui->rxModemTypeComboBox->currentIndex();
    _settings->tx_mode = ui->txModemTypeComboBox->currentIndex();
    _settings->ip_address = ui->lineEditIPaddress->text();
    _settings->rx_sample_rate = (long long)(ui->sampleRateBox->currentText().toInt());
    _settings->fft_size = (ui->fftSizeBox->currentText().toInt());
    _settings->scan_step = (int)ui->lineEditScanStep->text().toInt();
    _settings->waterfall_fps = (int)ui->fpsBox->currentText().toInt();
    _settings->saveConfig();
}

void MainWindow::addDisplayChannel(radiochannel *chan, int r)
{
    QTableWidgetItem *rx_freq_display = new QTableWidgetItem;
    QString rx_freq = QString("%L1").arg(chan->rx_frequency);
    rx_freq_display->setText(rx_freq);

    /// unused, using tx_shift
    //QTableWidgetItem *tx_freq_display = new QTableWidgetItem;
    //tx_freq_display->setText(QString::number(chan->tx_frequency));

    QTableWidgetItem *rx_mode_display = new QTableWidgetItem;
    rx_mode_display->setText(QString::number(chan->rx_mode));

    QTableWidgetItem *tx_mode_display = new QTableWidgetItem;
    tx_mode_display->setText(QString::number(chan->tx_mode));

    QTableWidgetItem *name_display = new QTableWidgetItem;
    name_display->setText(QString::fromStdString(chan->name));

    QTableWidgetItem *tx_shift_display = new QTableWidgetItem;
    tx_shift_display->setText(QString::number(chan->tx_shift / 1000));

    QTableWidgetItem *squelch_display = new QTableWidgetItem;
    squelch_display->setText(QString::number(chan->squelch));

    QTableWidgetItem *rx_volume_display = new QTableWidgetItem;
    rx_volume_display->setText(QString::number(chan->rx_volume));

    QTableWidgetItem *tx_power_display = new QTableWidgetItem;
    tx_power_display->setText(QString::number(chan->tx_power));

    QTableWidgetItem *rx_sensitivity_display = new QTableWidgetItem;
    rx_sensitivity_display->setText(QString::number(chan->rx_sensitivity));

    QTableWidgetItem *rx_ctcss_display = new QTableWidgetItem;
    rx_ctcss_display->setText(QString::number(chan->rx_ctcss));

    QTableWidgetItem *tx_ctcss_display = new QTableWidgetItem;
    tx_ctcss_display->setText(QString::number(chan->tx_ctcss));

    //QTableWidgetItem *id_display = new QTableWidgetItem;
    //id_display->setText(QString::number(chan->id));

    ui->memoriesTableWidget->insertRow(r);
    ui->memoriesTableWidget->setItem(r, 0, rx_freq_display);
    ui->memoriesTableWidget->setItem(r, 1, name_display);
    ui->memoriesTableWidget->setItem(r, 2, tx_shift_display);
    ui->memoriesTableWidget->setItem(r, 3, tx_mode_display);
    ui->memoriesTableWidget->setItem(r, 4, rx_mode_display);
    ui->memoriesTableWidget->setItem(r, 5, squelch_display);
    ui->memoriesTableWidget->setItem(r, 6, rx_volume_display);
    ui->memoriesTableWidget->setItem(r, 7, tx_power_display);
    ui->memoriesTableWidget->setItem(r, 8, rx_sensitivity_display);
    ui->memoriesTableWidget->setItem(r, 9, rx_ctcss_display);
    ui->memoriesTableWidget->setItem(r, 10, tx_ctcss_display);

    //ui->memoriesTableWidget->setItem(r, 11, id_display);
    //ui->memoriesTableWidget->setItem(r, 12, tx_freq_display);
    ui->memoriesTableWidget->horizontalHeader()->setHidden(false);
    ui->memoriesTableWidget->setStyleSheet(
        "color: rgb(240, 240, 119);background-color: rgb(0, 40, 102);font: 9pt \"Sans Serif\"; "\
        "QTableWidget {color: rgb(240, 240, 119);background-color: rgb(0, 40, 102);font: 9pt \"Sans Serif\";} "\
        "QTableWidget::item {color: rgb(240, 240, 119);background-color: rgb(0, 40, 102);font: 9pt \"Sans Serif\";} " \
        "QTableWidget::item:selected{ color: rgb(240, 240, 119);background-color: rgba(99, 0, 0, 125);"\
        "font: 9pt \"Sans Serif\"; }");

}

void MainWindow::updateMemories()
{
    for(int i=0;i<ui->memoriesTableWidget->rowCount();i++)
        ui->memoriesTableWidget->removeRow(i);
    ui->memoriesTableWidget->clearContents();
    ui->memoriesTableWidget->setRowCount(0);
    QVector<radiochannel*> *channels = _radio_channels->getChannels();
    int r;
    for(r=0;r<channels->size();r++)
    {
        radiochannel *chan = channels->at(r);
        addDisplayChannel(chan, r);
    }
    _new_mem_index = r;
}

void MainWindow::addMemoryChannel()
{
    radiochannel *chan = new radiochannel;
    chan->rx_frequency = _settings->rx_frequency + _settings->demod_offset;
    chan->tx_frequency = _settings->tx_frequency;
    chan->tx_shift = _settings->tx_shift;
    chan->rx_mode = _settings->rx_mode;
    chan->tx_mode = _settings->tx_mode;
    chan->squelch = _settings->squelch;
    chan->rx_volume = _settings->rx_volume;
    chan->tx_power = _settings->tx_power;
    chan->rx_sensitivity = _settings->rx_sensitivity;
    chan->rx_ctcss = _settings->rx_ctcss;
    chan->tx_ctcss = _settings->tx_ctcss;
    chan->id = _new_mem_index; // FIXME:
    chan->name = "";
    QVector<radiochannel*> *channels = _radio_channels->getChannels();
    channels->push_back(chan);
    addDisplayChannel(chan, _new_mem_index);
    _new_mem_index++;
}

void MainWindow::removeMemoryChannel()
{
    QVector<radiochannel*> *channels = _radio_channels->getChannels();
    QSet<int> row_list;
    QList<QTableWidgetItem *> items = ui->memoriesTableWidget->selectedItems();
    if(items.size()>0)
    {
        for(int i=0;i<items.size();i++)
        {
            row_list.insert(items.at(i)->row());
        }
    }
    QList<int> chan_to_remove = row_list.toList();
    std::sort(chan_to_remove.begin(), chan_to_remove.end());
    std::reverse(chan_to_remove.begin(), chan_to_remove.end());
    for(int i=0;i<chan_to_remove.size();i++)
    {
        radiochannel *chan = channels->at(chan_to_remove.at(i));
        channels->remove(chan_to_remove.at(i));
        delete chan;
    }
    updateMemories();
}

void MainWindow::tuneToMemoryChannel(int row, int col)
{
    Q_UNUSED(col);
    /*
    QList<QTableWidgetItem *> items = ui->memoriesTableWidget->selectedItems();
    for(int i =0;i<items.size();i++)
    {
        QTableWidgetItem *item = items.at(i);
        item->setBackgroundColor(QColor("#cc0000"));
        item->setTextColor(QColor("#ffffff"));
    }
    */

    QVector<radiochannel*> *channels = _radio_channels->getChannels();
    radiochannel *chan = channels->at(row);

    ui->frameCtrlFreq->setFrequency(chan->rx_frequency);
    tuneMainFreq(chan->rx_frequency);
    _settings->tx_shift = chan->tx_shift;
    emit changeTxShift(_settings->tx_shift);
    ui->shiftEdit->setText(QString::number(chan->tx_shift / 1000));

    ui->rxModemTypeComboBox->setCurrentIndex(chan->rx_mode);
    ui->txModemTypeComboBox->setCurrentIndex(chan->tx_mode);
    _settings->rx_mode = chan->rx_mode;
    _settings->tx_mode = chan->tx_mode;

    setSquelchDisplay(chan->squelch);
    setVolumeDisplay(chan->rx_volume);
    setTxPowerDisplay(chan->tx_power);
    setRxSensitivityDisplay(chan->rx_sensitivity);
    _settings->rx_ctcss = chan->rx_ctcss;
    _settings->tx_ctcss = chan->tx_ctcss;
    emit setRxCTCSS(chan->rx_ctcss);
    emit setTxCTCSS(chan->tx_ctcss);
    if(chan->rx_ctcss > 0.0)
        ui->comboBoxRxCTCSS->setCurrentText(QString::number(chan->rx_ctcss));
    else
        ui->comboBoxRxCTCSS->setCurrentText("CTCSS");
    if(chan->tx_ctcss > 0.0)
        ui->comboBoxTxCTCSS->setCurrentText(QString::number(chan->tx_ctcss));
    else
        ui->comboBoxTxCTCSS->setCurrentText("CTCSS");


}

void MainWindow::editMemoryChannel(QTableWidgetItem* item)
{
    int row = item->row();
    int col = item->column();
    QVector<radiochannel*> *channels = _radio_channels->getChannels();
    radiochannel *chan = channels->at(row);
    switch(col)
    {
    case 0:
        chan->rx_frequency = (long)(item->text().replace(",", "").toInt());
        break;
    case 1:
        chan->name = item->text().toStdString();
        break;
    case 2:
        chan->tx_shift = item->text().toInt() * 1000;
        break;
    case 3:
        chan->rx_mode = item->text().toInt();
        break;
    case 4:
        chan->tx_mode = item->text().toInt();
        break;
    case 5:
        chan->squelch = item->text().toInt();
        break;
    case 6:
        chan->rx_volume = item->text().toInt();
        break;
    case 7:
        chan->tx_power = item->text().toInt();
        break;
    case 8:
        chan->rx_sensitivity = item->text().toInt();
        break;
    case 9:
        chan->rx_ctcss = item->text().toFloat();
        break;
    case 10:
        chan->tx_ctcss = item->text().toFloat();
        break;
    }
}

void MainWindow::saveMemoryChannes()
{
    _radio_channels->saveConfig();
}


void MainWindow::endTx()
{
    emit endTransmission();
}

void MainWindow::startTx()
{
    if(!_ptt_activated)
    {
        emit startTransmission();
        ui->frameCtrlFreq->setFrequency(_settings->rx_frequency + _settings->demod_offset + _settings->tx_shift, false);
        _ptt_activated=true;
    }
    else
    {
        ui->frameCtrlFreq->setFrequency(_settings->rx_frequency + _settings->demod_offset, false);
        _ptt_activated=false;
        endTx();
    }
}

void MainWindow::sendTextRequested()
{
    QString text = ui->sendTextEdit->toPlainText();
    emit sendText(text, false);
    ui->sendTextEdit->setPlainText("");
}

void MainWindow::sendMumbleTextMessage()
{
    QString text = ui->mumbleTextMessageEdit->text();
    if(text.size() < 1)
        return;
    emit newMumbleMessage(text);
    ui->mumbleTextMessageEdit->setText("");
}

void MainWindow::newFFTData(float *fft_data, int fftsize)
{
    // don't paint anything if window is minimized
    if(isMinimized())
        return;

    if (fftsize == 0)
        return;

    for (int i = 0; i < fftsize; i++)
    {
        _realFftData[i] = fft_data[i];
        /// FFT averaging
        if(_settings->fft_averaging < 0.99f)
            _iirFftData[i] += _settings->fft_averaging * (_realFftData[i] - _iirFftData[i]);
        else
            _iirFftData[i] = _realFftData[i];

    }
    ui->plotterFrame->setNewFftData(_iirFftData, _realFftData, fftsize);
}


void MainWindow::setFFTSize(int size)
{
    Q_UNUSED(size);
    emit newFFTSize(ui->fftSizeBox->currentText().toInt());
}

void MainWindow::setAveraging(int x)
{
    _settings->fft_averaging = 1.0 / x;
}

void MainWindow::newWaterfallFPS()
{
    _settings->waterfall_fps = ui->fpsBox->currentText().toInt();
    emit setWaterfallFPS(_settings->waterfall_fps);
}

void MainWindow::updateConstellation(complex_vector *constellation_data)
{
    if(isMinimized())
    {
        return;
    }
    _mutex.lock();
    _constellation_img->fill(QColor("transparent"));
    QPen pen(QColor(0,255,0,255), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen pen2(QColor(180,180,180,180), 1, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);

    _constellation_painter->begin(_constellation_img);
    _constellation_painter->setCompositionMode(QPainter::CompositionMode_Source);
    _constellation_painter->setPen(pen2);
    _constellation_painter->drawLine(150, 0, 150, 300);
    _constellation_painter->drawLine(0, 150, 300, 150);
    _constellation_painter->setPen(pen);
    for(int i = 0;i < (int)constellation_data->size();i++)
    {
        std::complex<float> pt = constellation_data->at(i);
        int x = (int)(floor((std::min(pt.real(), 2.0f) * 75)) + 150);
        int y = (int)(floor((std::min(pt.imag(), 2.0f) * 75)) + 150);
        _constellation_painter->drawPoint(x, y);
    }
    _constellation_painter->end();

    ui->constellationLabel->setPixmap(*_constellation_img);
    _mutex.unlock();
    constellation_data->clear();
    delete constellation_data;
}

void MainWindow::displayText(QString text, bool html)
{
    ui->receivedTextEdit->moveCursor(QTextCursor::End);
    if(ui->receivedTextEdit->toPlainText().size() > 1024*1024*1024)
    {
        ui->receivedTextEdit->clear();
    }
    if(html)
        ui->receivedTextEdit->insertHtml(text);
    else
        ui->receivedTextEdit->insertPlainText(text);

    ui->receivedTextEdit->verticalScrollBar()->setValue(
                ui->receivedTextEdit->verticalScrollBar()->maximum());

    // text widget
    ui->secondaryTextDisplay->moveCursor(QTextCursor::End);
    if(ui->secondaryTextDisplay->toPlainText().size() > 1024*1024*1024)
    {
        ui->secondaryTextDisplay->clear();
    }
    if(html)
        ui->secondaryTextDisplay->insertHtml(text);
    else
        ui->secondaryTextDisplay->insertPlainText(text);

    ui->secondaryTextDisplay->verticalScrollBar()->setValue(
                ui->secondaryTextDisplay->verticalScrollBar()->maximum());
    ui->secondaryTextDisplay->show();
    _secondary_text_timer.start(10000);
}

void MainWindow::displayVOIPText(QString text, bool html)
{
    ui->voipMessagesEdit->moveCursor(QTextCursor::End);
    if(ui->voipMessagesEdit->toPlainText().size() > 1024*1024*1024)
    {
        ui->voipMessagesEdit->clear();
    }
    if(html)
        ui->voipMessagesEdit->insertHtml(text);
    else
        ui->voipMessagesEdit->insertPlainText(text);

    ui->voipMessagesEdit->verticalScrollBar()->setValue(
                ui->voipMessagesEdit->verticalScrollBar()->maximum());
}

void MainWindow::clearTextArea()
{
    ui->receivedTextEdit->setPlainText("");
    ui->receivedTextEdit->moveCursor(QTextCursor::End);
    ui->receivedTextEdit->verticalScrollBar()->setValue(
                ui->receivedTextEdit->verticalScrollBar()->maximum());
}

void MainWindow::displayImage(QImage img)
{
    ui->videoLabel->clear();
    delete _video_img;
    _video_img = new QPixmap(QPixmap::fromImage(img,Qt::AutoColor));
    ui->videoLabel->setPixmap(*_video_img);
    ui->videoFrame->show();
    _video_timer.start(3000);
}

void MainWindow::displayCallsign(QString callsign)
{
    ui->labelDisplayCallsign->setText(callsign);
}

void MainWindow::chooseFile()
{
    // FIXME: this sets repeat on and only works with text files
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "~/", tr("All Files (*.*)"));
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QString filedata = QString(file.readAll().constData());
    emit sendText(filedata, true);
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

void MainWindow::connectVOIPRequested()
{
    _settings->voip_server = ui->voipServerEdit->text();
    _settings->voip_port = ui->voipServerPortEdit->text().toInt();
    _settings->voip_password = ui->voipPasswordEdit->text();
    emit connectToServer(ui->voipServerEdit->text(), ui->voipServerPortEdit->text().toInt());
    emit setMute(false); // FIXME: ???
}

void MainWindow::disconnectVOIPRequested()
{
    emit disconnectFromServer();
    ui->voipTreeWidget->clear();
    ui->voipConnectButton->setDisabled(false);
}

void MainWindow::disconnectedFromServer()
{
    ui->voipTreeWidget->clear();
    ui->voipConnectButton->setDisabled(false);
}

void MainWindow::connectedToServer(QString msg)
{
    displayVOIPText(msg, false);
    ui->voipConnectButton->setDisabled(true);
}


void MainWindow::updateOnlineStations(StationList stations)
{
    _user_list = stations;
    // FIXME: code below is unmaintainable!
    for(int i=0;i<stations.size();i++)
    {
        QList<QTreeWidgetItem*> list = ui->voipTreeWidget->findItems(
                    ".", Qt::MatchRegExp | Qt::MatchExactly | Qt::MatchRecursive,3);
        if(list.size()>0)
        {
            delete list.at(0);
        }
    }
    for(int i=0;i<stations.size();i++)
    {
        QList<QTreeWidgetItem*> channel_list = ui->voipTreeWidget->findItems(
                    QString::number(stations.at(i)->channel_id),
                    Qt::MatchExactly | Qt::MatchRecursive,2);
        if(channel_list.size()>0)
        {
            if(QString::number(stations.at(i)->id) == ".")
                continue;
            QTreeWidgetItem *item = channel_list.at(0);
            QTreeWidgetItem *st_item = new QTreeWidgetItem(0);
            st_item->setText(0,stations.at(i)->callsign);
            st_item->setText(3,QString::number(stations.at(i)->id));
            st_item->setIcon(0,QIcon(":/res/im-user.png"));
            st_item->setBackgroundColor(0,QColor("#ffffff"));
            st_item->setBackgroundColor(1,QColor("#ffffff"));
            st_item->setBackgroundColor(2,QColor("#ffffff"));
            st_item->setBackgroundColor(3,QColor("#ffffff"));
            if(stations.at(i)->is_user)
                st_item->setTextColor(0,QColor("#cc0000"));
            item->addChild(st_item);
        }
    }
}


void MainWindow::userSpeaking(quint64 id)
{
    QList<QTreeWidgetItem*> list = ui->voipTreeWidget->findItems(QString::number(id),
             Qt::MatchExactly | Qt::MatchRecursive,3);
    if(list.size()>0)
    {
        list.at(0)->setIcon(0, QIcon(":res/text-speak.png"));
    }
    if(!_speech_icon_timer.isActive())
        _speech_icon_timer.start(1000);
}

void MainWindow::resetSpeechIcons()
{
    for(int i =0;i<_user_list.size();i++)
    {
        QList<QTreeWidgetItem*> list = ui->voipTreeWidget->findItems(
                    QString::number(_user_list.at(i)->id),
                 Qt::MatchExactly | Qt::MatchRecursive,3);
        if(list.size()>0)
        {
            list.at(0)->setIcon(0, QIcon(":res/im-user.png"));
        }
    }
}

void MainWindow::updateChannels(ChannelList channels)
{
    // FIXME: code below is unmaintainable!
    ui->voipTreeWidget->clear();
    for(int i = 0;i< channels.size();i++)
    {
        MumbleChannel *chan = channels.at(i);

        /// Channel we're in
        if(chan->name.isEmpty())
        {
            continue;
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
        t->setIcon(0,QIcon(":/res/call-start.png"));

        if(chan->parent_id <= 0)
            ui->voipTreeWidget->addTopLevelItem(t);
        else
        {
            QList<QTreeWidgetItem*> channel_list = ui->voipTreeWidget->findItems(
                        QString::number(chan->parent_id),Qt::MatchExactly | Qt::MatchRecursive,2);
            if(channel_list.size() > 0)
            {
                QTreeWidgetItem *parent = channel_list.at(0);
                parent->addChild(t);
                t->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
                t->setIcon(0,QIcon(":/res/call-start.png"));
            }
        }
    }
    ui->voipTreeWidget->expandAll();
}

void MainWindow::joinedChannel(quint64 channel_id)
{
    // FIXME: code below is unmaintainable!
    QList<QTreeWidgetItem*> old_channel_list = ui->voipTreeWidget->findItems(
                QString::number(_current_voip_channel),
                Qt::MatchExactly | Qt::MatchRecursive,2);
    if(old_channel_list.size() > 0)
    {
        QTreeWidgetItem *t = old_channel_list.at(0);
        t->setBackgroundColor(0,QColor("#ffffff"));
        t->setBackgroundColor(1,QColor("#ffffff"));
        t->setBackgroundColor(2,QColor("#ffffff"));
        t->setTextColor(0,QColor("#000000"));
        t->setTextColor(1,QColor("#000000"));
        t->setTextColor(2,QColor("#000000"));
        t->setIcon(0,QIcon(":/res/call-start.png"));
    }


    QList<QTreeWidgetItem*> channel_list = ui->voipTreeWidget->findItems(
                QString::number(channel_id),Qt::MatchExactly | Qt::MatchRecursive,2);
    if(channel_list.size() > 0)
    {
        QTreeWidgetItem *t = channel_list.at(0);
        t->setBackgroundColor(0,QColor("#cc0000"));
        t->setBackgroundColor(1,QColor("#cc0000"));
        t->setBackgroundColor(2,QColor("#cc0000"));
        t->setTextColor(0,QColor("#ffffff"));
        t->setTextColor(1,QColor("#ffffff"));
        t->setTextColor(2,QColor("#ffffff"));
        t->setIcon(0,QIcon(":/res/call-start.png"));
    }
    _current_voip_channel = channel_id;
    ui->voipTreeWidget->expandAll();
}

void MainWindow::channelState(QTreeWidgetItem *item, int k)
{
    Q_UNUSED(k);
    emit changeChannel((int)item->data(2,0).toInt());
}

void MainWindow::toggleSelfDeaf(bool deaf)
{
    bool mute = ui->muteSelfButton->isChecked();
    emit setSelfDeaf(deaf, mute);
}
void MainWindow::toggleSelfMute(bool mute)
{
    emit setSelfMute(mute);
}

void MainWindow::toggleRXwin(bool value)
{
    emit setSampleRate(ui->sampleRateBox->currentText().toInt());
    emit toggleRX(value);
    ui->plotterFrame->setRunningState(value);
    setFFTSize(_settings->fft_size);
    newWaterfallFPS();
    _range_set = false;
}

void MainWindow::toggleTXwin(bool value)
{
    emit toggleTX(value);
}

void MainWindow::toggleWideband(bool value)
{
    // ????
    emit toggleWidebandMode(value);
}

void MainWindow::setFilterWidth(int index)
{
    std::complex<int> widths = _filter_widths->at(index);
    std::complex<int> ranges = _filter_ranges->at(index);
    bool symmetric = _filter_symmetric->at(index);
    ui->plotterFrame->setDemodRanges(ranges.real(),ranges.imag(),ranges.real(),ranges.imag(),symmetric);
    ui->plotterFrame->setHiLowCutFrequencies(widths.real(), widths.imag());
    _filter_low_cut = ranges.real();
    _filter_high_cut = ranges.imag();
    _filter_is_symmetric = symmetric;
}

void MainWindow::toggleRxMode(int value)
{
    _settings->rx_mode = value;
    emit toggleRxModemMode(value);
    setFilterWidth(value);
}

void MainWindow::toggleTxMode(int value)
{
    _settings->tx_mode = value;
    emit toggleTxModemMode(value);
}

void MainWindow::initError(QString error)
{
    Q_UNUSED(error);
    ui->tabWidget->setCurrentIndex(3);
}

void MainWindow::toggleRepeater(bool value)
{
    emit toggleRepeat(value);
}

void MainWindow::clarifierTuneFreq(int value)
{
    Q_UNUSED(value);
    emit fineTuneFreq((int)ui->tuneDial->value());
}

void MainWindow::tuneMainFreq(qint64 freq)
{

    ui->frequencyEdit->setText(QString::number(ceil(freq/1000)));
    ui->tuneDial->setValue(0);
    /// rx_frequency is the center frequency of the source
    _settings->rx_frequency = freq - _settings->demod_offset;
    /// tx_frequency is the actual frequency
    _settings->tx_frequency = freq;
    ui->plotterFrame->setCenterFreq(_settings->rx_frequency);
    ui->plotterFrame->setDemodCenterFreq(_settings->rx_frequency + _settings->demod_offset);
    emit setCarrierOffset(_settings->demod_offset);
    emit tuneFreq(_settings->rx_frequency);
    emit tuneTxFreq(freq);
}


// this happens on middle mouse click
void MainWindow::tuneFreqPlotter(qint64 freq)
{
    return; // can't handle this now
    _settings->tx_frequency = freq;
    tuneMainFreq(freq - _settings->demod_offset);
    ui->frameCtrlFreq->setFrequency(freq + _settings->demod_offset);
}

void MainWindow::carrierOffsetChanged(qint64 freq, qint64 offset)
{
    _settings->demod_offset = offset;
    ui->frameCtrlFreq->setFrequency(_settings->rx_frequency + _settings->demod_offset, false);
    emit setCarrierOffset(offset);
    emit tuneTxFreq(freq);
}

void MainWindow::enterFreq()
{
    ui->frameCtrlFreq->setFrequency(ui->frequencyEdit->text().toLong()*1000);
    _settings->rx_frequency = ui->frequencyEdit->text().toLong()*1000 - _settings->demod_offset;
    emit tuneFreq(_settings->rx_frequency);
}

void MainWindow::enterShift()
{
    _settings->tx_shift = ui->shiftEdit->text().toLong()*1000;
    emit changeTxShift(_settings->tx_shift);
}

void MainWindow::setTxPowerDisplay(int value)
{
    _settings->tx_power = value;
    ui->txPowerDisplay->display(value);
    ui->txGainDial->setValue(value);
    emit setTxPower((int)value);
}

void MainWindow::setRxSensitivityDisplay(int value)
{
    _settings->rx_sensitivity = value;
    ui->rxSensitivityDisplay->display(value);
    ui->rxGainDial->setValue(value);
    emit setRxSensitivity((int)value);
}

void MainWindow::setSquelchDisplay(int value)
{
    _settings->squelch = value;
    ui->rxSquelchDisplay->display(value);
    ui->rxSquelchDial->setValue(value);
    emit setSquelch(value);
}

void MainWindow::setVolumeDisplay(int value)
{
    _settings->rx_volume = value;
    ui->rxVolumeDisplay->display(value);
    ui->rxVolumeDial->setValue(value);
    emit setVolume((int)value);
}

void MainWindow::setTxVolumeDisplay(int value)
{
    _settings->tx_volume = value;
    ui->micGainSlider->setSliderPosition(value);
    emit setTxVolume((int)value);
}

void MainWindow::changeVoipVolume(int value)
{
    _settings->voip_volume = value;
    ui->voipGainSlider->setSliderPosition(value);
    emit setVoipVolume((int)value);
}

void MainWindow::startScan(bool value)
{
    int scan_direction = 0;
    QObject *which_button = this->sender();
    if(which_button == ui->scanUpButton)
        scan_direction = 1;
    if(value)
    {
        int step = ui->lineEditScanStep->text().toInt();
        emit startAutoTuneFreq(step, scan_direction);
    }
    else
        emit stopAutoTuneFreq();
}

void MainWindow::startMemoryScan(bool value)
{
    int scan_direction = 0;
    QObject *which_button = this->sender();
    if(which_button == ui->memoryScanUpButton)
        scan_direction = 1;
    if(value)
    {

        emit startMemoryTune(scan_direction);
    }
    else
        emit stopMemoryTune();
}


void MainWindow::mainTabChanged(int value)
{
    Q_UNUSED(value);
}

void MainWindow::updateFreqGUI(long long center_freq, long carrier_offset)
{
    // Lots of signals flowing around
    _settings->demod_offset = carrier_offset;
    _settings->rx_frequency = (qint64)center_freq;
    ui->frameCtrlFreq->setFrequency(_settings->rx_frequency + _settings->demod_offset, false);
    ui->plotterFrame->setFilterOffset((qint64)_settings->demod_offset);
    ui->plotterFrame->setCenterFreq(_settings->rx_frequency);
    ui->frequencyEdit->setText(QString::number(
                    ceil((_settings->rx_frequency + _settings->demod_offset)/1000)));
}

void MainWindow::updateRxCTCSS(int value)
{
    Q_UNUSED(value);
    _settings->rx_ctcss = ui->comboBoxRxCTCSS->currentText().toFloat();
    emit setRxCTCSS(_settings->rx_ctcss);
}

void MainWindow::updateTxCTCSS(int value)
{
    Q_UNUSED(value);
    _settings->tx_ctcss = ui->comboBoxTxCTCSS->currentText().toFloat();
    emit setTxCTCSS(_settings->tx_ctcss);
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

void MainWindow::setPeakDetect(bool value)
{
    ui->plotterFrame->setPeakDetection(value, 6.0);
}

void MainWindow::updateRSSI(float value)
{
    _rssi = value;
    if(!_range_set)
    {
        //setFFTRange(1);
        _range_set = true;
    }
    if(!_settings->show_controls && value > 90.0)
        return;

    //float S9 = 80.0; // degrees
    float arc_min = 135.0;
    float arc_max = 45.0;
    float abs_rssi = fabs(_rssi);
    if(abs_rssi > arc_min)
    {
        abs_rssi = arc_min; // should be 0 on the scale
    }
    if(abs_rssi < arc_max)
    {
        abs_rssi = arc_max;
    }
    int deviation = (int) ((arc_min - arc_max) / 2*M_PI*abs_rssi);
    if(abs_rssi > 90.0)
        deviation = -deviation;

    ui->labelRSSI->setText(QString::number(value));
    QLineF needle;
    needle.setP1(QPointF(77,105));
    needle.setAngle(abs_rssi);
    needle.setLength(100 + (int)(abs(90-abs_rssi)/10));
    QPen pen(QColor(224,33,33,255), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPixmap s_meter = _s_meter_bg->copy(0,0,154,60);
    QPainter p(&s_meter);
    p.setRenderHint(QPainter::HighQualityAntialiasing);
    p.setPen(pen);
    p.drawLine(needle);
    p.end();
    ui->labelSMeter->setPixmap(s_meter);
}

void MainWindow::updateSampleRate()
{
    int samp_rate = ui->sampleRateBox->currentText().toInt();
    _settings->rx_sample_rate = samp_rate;
    ui->plotterFrame->setSampleRate(samp_rate);
    ui->plotterFrame->setSpanFreq((quint32)samp_rate);
    emit setSampleRate(samp_rate);
}

void MainWindow::setFFTRange(int value)
{
    /// value is from one to 10
    if(_rssi == 0)
        _rssi = -80.0;
    ui->plotterFrame->setPandapterRange(_rssi - 20 / (float)value , _rssi + 70 / (float)value);
    ui->plotterFrame->setWaterfallRange(_rssi - 20 / (float)value , _rssi + 70 / (float)value);
}

void MainWindow::autoSquelch()
{
    if(_rssi == 0) // FIXME: float?
        return;
    int calibration = ui->rssiCalibrateEdit->text().toInt();
    int squelch = (int)_rssi + (abs(calibration) - 80) + 50;
    setSquelchDisplay(squelch);
}

void MainWindow::changeFilterWidth(int low, int up)
{
    if(_filter_is_symmetric)
    {
        int abs_limit_lower = 800;
        int abs_width = std::max(abs(low), abs(up));
        if((low >= _filter_low_cut) && (abs_width >= abs_limit_lower))
        {
            emit newFilterWidth(abs_width);
        }
        else if((up <= _filter_high_cut) && (abs_width >= abs_limit_lower))
        {
            emit newFilterWidth(abs_width);
        }
    }
    else
    {
        int abs_width = std::max(abs(low), abs(up));
        int abs_limit_upper = std::max(abs(_filter_low_cut), abs(_filter_high_cut));
        int abs_limit_lower = 800;
        if((abs_width <= abs_limit_upper) && (abs_width >= abs_limit_lower))
        {
            emit newFilterWidth(abs_width);
        }
    }
}

void MainWindow::setAudioCompressor(bool value)
{
    _settings->audio_compressor = (int)value;
     emit enableAudioCompressor(value);
}

void MainWindow::setRelays(bool value)
{
    _settings->enable_relays = (int) value;
    emit enableRelays(value);
}

void MainWindow::setRemoteControl(bool value)
{
    _settings->remote_control = (int) value;
    if(value)
        emit enableRemote();
    else
        emit disableRemote();
}

void MainWindow::setRSSICalibration()
{
    int value = ui->rssiCalibrateEdit->text().toInt();
    _settings->rssi_calibration_value = (int) value;
    emit calibrateRSSI((float) value);
}

void MainWindow::setDigitalGain(int value)
{
    _settings->bb_gain = value;
    emit setBbGain(value);
}

void MainWindow::updateAgcAttack(int value)
{
    Q_UNUSED(value);
    _settings->agc_attack = value;
    float attack = (float)pow(10, -value);
    emit setAgcAttack(attack);
}

void MainWindow::updateAgcDecay(int value)
{
    Q_UNUSED(value);
    _settings->agc_decay = value;
    float decay = (float)pow(10, -value);
    emit setAgcDecay(decay);
}

void MainWindow::setRxGainStages(gain_vector rx_gains)
{
    QMap<std::string, QVector<int>>::const_iterator iter = rx_gains.constBegin();
    while (iter != rx_gains.constEnd())
    {
        QString gain_stage_name = QString::fromStdString(iter.key());
        QSlider *gain_slider = new QSlider(Qt::Horizontal, this);
        gain_slider->setObjectName(gain_stage_name);
        gain_slider->setRange(iter.value().at(0), iter.value().at(1));
        gain_slider->setMaximumWidth(150);
        gain_slider->setTickInterval(10);
        _rx_gain_sliders.push_back(gain_slider);
        ++iter;
    }
}

void MainWindow::setTxGainStages(gain_vector tx_gains)
{
    QMap<std::string, QVector<int>>::const_iterator iter = tx_gains.constBegin();
    while (iter != tx_gains.constEnd())
    {
        QString gain_stage_name = QString::fromStdString(iter.key());
        QSlider *gain_slider = new QSlider(Qt::Horizontal, this);
        gain_slider->setObjectName(gain_stage_name);
        gain_slider->setRange(iter.value().at(0), iter.value().at(1));
        gain_slider->setMaximumWidth(150);
        gain_slider->setTickInterval(10);
        _tx_gain_sliders.push_back(gain_slider);
        ++iter;
    }
}
