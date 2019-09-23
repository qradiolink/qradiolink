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


MainWindow::MainWindow(Settings *settings, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    setAnimated(true);
    ui->setupUi(this);
    _settings = settings;
    static float tone_list[]= {67.0, 71.9, 74.4, 77.0, 79.7, 82.5, 85.4, 88.5, 81.5, 87.4, 94.8, 100.0, 103.5, 107.2, 110.9,
                         114.8, 118.8, 123.0, 127.3, 131.8, 136.5, 141.3, 146.2, 151.4, 156.7, 162.2,
                          167.9, 173.8, 179.9, 186.2, 192.8, 203.5, 210.7, 218.1, 225.7, 233.6, 241.8, 250.3};
    QStringList tones;
    tones.append("CTCSS");
    for(int i=0;i<38;i++)
    {
        tones.append(QString::number(tone_list[i]));
    }

    // FIXME: should probably use a map or something and keep track of them elsewhere
    _filter_widths = new std::vector<std::complex<int>>;
    _filter_widths->push_back(std::complex<int>(-5000, 5000));  // FM
    _filter_widths->push_back(std::complex<int>(-2500, 2500));  // NBFM
    _filter_widths->push_back(std::complex<int>(-100000, 100000));  // WFM
    _filter_widths->push_back(std::complex<int>(-1, 2500)); // USB
    _filter_widths->push_back(std::complex<int>(-2500, 1)); // LSB
    _filter_widths->push_back(std::complex<int>(-1, 2500)); // FreeDV1600 USB
    _filter_widths->push_back(std::complex<int>(-1, 2500)); // FreeDV700C USB
    _filter_widths->push_back(std::complex<int>(-2500, 1)); // FreeDV1600 LSB
    _filter_widths->push_back(std::complex<int>(-2500, 1)); // FreeDV700C LSB
    _filter_widths->push_back(std::complex<int>(-5000, 5000));  // AM
    _filter_widths->push_back(std::complex<int>(-2800, 2800)); // BPSK 2K
    _filter_widths->push_back(std::complex<int>(-1400, 1400)); // BPSK 700
    _filter_widths->push_back(std::complex<int>(-1500, 1500));  // QPSK 2K
    _filter_widths->push_back(std::complex<int>(-7000, 7000));    // QPSK 10K
    _filter_widths->push_back(std::complex<int>(-4600, 4600));  // 2FSK 2K
    _filter_widths->push_back(std::complex<int>(-15000, 15000));  // 2FSK 10K
    _filter_widths->push_back(std::complex<int>(-4600, 4600));  // 4FSK 2K
    _filter_widths->push_back(std::complex<int>(-25000, 25000));    // 4FSK 10K
    _filter_widths->push_back(std::complex<int>(-150000, 150000)); // QPSK250000 VIDEO
    _filter_widths->push_back(std::complex<int>(-150000, 150000)); // QPSK250000 DATA



    ui->comboBoxTxCTCSS->addItems(tones);
    ui->comboBoxRxCTCSS->addItems(tones);

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

    _speech_icon_timer.setSingleShot(true);
    _secondary_text_timer.setSingleShot(true);

    QObject::connect(ui->buttonTransmit,SIGNAL(toggled(bool)),this,SLOT(startTx()));
    //QObject::connect(ui->buttonTransmit,SIGNAL(released()),this,SLOT(GUIendTransmission()));
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
    QObject::connect(ui->rxVolumeDial,SIGNAL(valueChanged(int)),this,SLOT(setVolumeDisplay(int)));
    QObject::connect(ui->rxModemTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleRxMode(int)));
    QObject::connect(ui->txModemTypeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleTxMode(int)));
    QObject::connect(ui->scanUpButton,SIGNAL(toggled(bool)),this,SLOT(startScan(bool)));
    QObject::connect(ui->scanDownButton,SIGNAL(toggled(bool)),this,SLOT(startScan(bool)));
    QObject::connect(ui->saveOptionsButton,SIGNAL(clicked()),this,SLOT(saveConfig()));
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


    QObject::connect(ui->frameCtrlFreq,SIGNAL(newFrequency(qint64)),this,SLOT(tuneMainFreq(qint64)));
    QObject::connect(ui->plotterFrame,SIGNAL(pandapterRangeChanged(float,float)),ui->plotterFrame,SLOT(setWaterfallRange(float,float)));
    QObject::connect(ui->plotterFrame,SIGNAL(newCenterFreq(qint64)),this,SLOT(tuneFreqPlotter(qint64)));
    QObject::connect(ui->panadapterSlider,SIGNAL(valueChanged(int)),ui->plotterFrame,SLOT(setPercent2DScreen(int)));
    QObject::connect(ui->averagingSlider,SIGNAL(valueChanged(int)),this,SLOT(setAveraging(int)));
    QObject::connect(ui->rangeSlider,SIGNAL(valueChanged(int)),this,SLOT(setFFTRange(int)));
    QObject::connect(ui->plotterFrame,SIGNAL(newDemodFreq(qint64,qint64)),this,SLOT(carrierOffsetChanged(qint64,qint64)));

    QObject::connect(ui->voipTreeWidget,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT(channelState(QTreeWidgetItem *,int)));
    QObject::connect(&_secondary_text_timer,SIGNAL(timeout()),ui->secondaryTextDisplay,SLOT(hide()));
    QObject::connect(&_video_timer,SIGNAL(timeout()),ui->videoFrame,SLOT(hide()));
    QObject::connect(&_speech_icon_timer,SIGNAL(timeout()),this,SLOT(resetSpeechIcons()));

    ui->rxModemTypeComboBox->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->txModemTypeComboBox->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->sendTextEdit->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->receivedTextEdit->setAttribute(Qt::WA_AcceptTouchEvents);
    ui->voipTreeWidget->setColumnHidden(2,true);
    ui->voipTreeWidget->setColumnHidden(3,true);
    _transmitting_radio = false;
    ui->controlsFrame->hide();
    ui->constellationDisplay->hide();
    ui->secondaryTextDisplay->hide();
    ui->videoFrame->hide();
    _current_voip_channel = -1;
    _demod_offset = 0;

    _video_img = new QPixmap;
    _constellation_img = new QPixmap(300,300);
    ui->menuBar->hide();
    ui->statusBar->hide();
    ui->mainToolBar->hide();
    setWindowIcon(QIcon(":/res/logo.png"));
    _realFftData = new float[1024*1024];
    _pwrFftData = new float[1024*1024];
    _iirFftData = new float[1024*1024];
    _fft_averaging = 1;
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
    ui->plotterFrame->setFreqDigits(2);
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

    _s_meter_bg = new QPixmap(":/res/s-meter-bg-black-small.png");

}

void MainWindow::initSettings()
{
    readConfig();
    updateRSSI(9999);
    _range_set = false;
    //setFFTRange(1);
    setEnabledFFT((bool)_settings->show_fft);
    setEnabledDuplex((bool) _settings->enable_duplex);
    _range_set = false;
    ui->showConstellationButton->setChecked(_settings->show_constellation);
    showConstellation(_settings->show_constellation);
    ui->showControlsButton->setChecked((bool)_settings->show_controls);
    showControls((bool)_settings->show_controls);

}

MainWindow::~MainWindow()
{
    delete ui;
    _filter_widths->clear();
    delete _filter_widths;
    delete _video_img;
    if(_constellation_painter->isActive())
        _constellation_painter->end();
    delete _constellation_img;
    delete _constellation_painter;
    delete[] _realFftData;
    delete[] _pwrFftData;
    delete[] _iirFftData;
    delete _eff_freq;
    delete _eff_const;
    delete _eff_video;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    saveConfig();
    emit stopRadio();
    emit disconnectFromServer();
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
    }
    else
    {
        ui->plotterContainer->resize(xy.right() -xy.left()-20,xy.bottom()-xy.top()-120);
        xy = ui->plotterContainer->geometry();
        ui->secondaryTextDisplay->move(xy.left(), xy.bottom() - 150);
    }
    xy = ui->plotterContainer->geometry();
    ui->videoFrame->move(xy.right() - 360, xy.top());
    event->accept();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (isMinimized())
        {
            emit enableGUIFFT(false);
            emit enableGUIConst(false);
            emit enableRSSI(false);
        }
        else
        {
            emit enableGUIFFT(_settings->show_fft);
            emit enableGUIConst(_settings->show_constellation);
            emit enableRSSI(_settings->show_controls);
        }
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
        _settings->show_controls = 1;
    }
    else
    {
        ui->plotterContainer->resize(xy.right() -xy.left()-20,xy.bottom()-xy.top()-120);
        ui->controlsFrame->hide();
        xy = ui->plotterContainer->geometry();
        ui->secondaryTextDisplay->move(xy.left(), xy.bottom() - 150);
        _settings->show_controls = 0;
    }
    emit enableRSSI(value);
}

void MainWindow::showConstellation(bool value)
{
    if(value)
    {
        ui->constellationDisplay->show();
        _settings->show_constellation = 1;
    }
    else
    {
        ui->constellationDisplay->hide();
        _settings->show_constellation = 0;
    }
    emit enableGUIConst(value);
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

    _rx_frequency = _settings->rx_frequency;
    _demod_offset = _settings->demod_offset;
    _rx_sample_rate = _settings->rx_sample_rate;

    ui->frequencyEdit->setText(QString::number(ceil(_rx_frequency/1000)));
    _tx_shift_frequency = _settings->tx_shift;
    ui->shiftEdit->setText(QString::number(_tx_shift_frequency / 1000));
    ui->voipServerEdit->setText(_settings->voip_server);
    ui->rxModemTypeComboBox->setCurrentIndex(_settings->rx_mode);
    ui->txModemTypeComboBox->setCurrentIndex(_settings->tx_mode);
    ui->lineEditIPaddress->setText(_settings->ip_address);
    ui->plotterFrame->setFilterOffset((qint64)_settings->demod_offset);
    ui->plotterFrame->setCenterFreq(_rx_frequency);
    ui->frameCtrlFreq->setFrequency(_rx_frequency + _demod_offset);
    ui->plotterFrame->setSampleRate(_settings->rx_sample_rate);
    ui->plotterFrame->setSpanFreq((quint32)_settings->rx_sample_rate);
    ui->sampleRateBox->setCurrentIndex(ui->sampleRateBox->findText(QString::number(_settings->rx_sample_rate)));
    ui->fftSizeBox->setCurrentIndex(ui->fftSizeBox->findText(QString::number(_settings->fft_size)));
    ui->fpsBox->setCurrentIndex(ui->fpsBox->findText(QString::number(_settings->waterfall_fps)));
    ui->lineEditScanStep->setText(QString::number(_settings->scan_step));
    ui->fftEnableCheckBox->setChecked((bool)_settings->show_fft);
    ui->duplexOpButton->setChecked((bool) _settings->enable_duplex);

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
    _settings->tx_shift = _tx_shift_frequency;
    _settings->voip_server = ui->voipServerEdit->text();
    _settings->rx_mode = ui->rxModemTypeComboBox->currentIndex();
    _settings->tx_mode = ui->txModemTypeComboBox->currentIndex();
    _settings->ip_address = ui->lineEditIPaddress->text();
    _settings->demod_offset = (long long)_demod_offset;
    _settings->rx_sample_rate = (long long)(ui->sampleRateBox->currentText().toInt());
    _settings->fft_size = (long long)(ui->fftSizeBox->currentText().toInt());
    _settings->scan_step = (int)ui->lineEditScanStep->text().toInt();
    _settings->waterfall_fps = (int)ui->fpsBox->currentText().toInt();
    _settings->saveConfig();
}


void MainWindow::endTx()
{
    emit endTransmission();
}

void MainWindow::startTx()
{
    if(!_transmitting_radio)
    {
        emit startTransmission();
        ui->frameCtrlFreq->setFrequency(_rx_frequency + _demod_offset + _tx_shift_frequency, false);
        _transmitting_radio=true;
    }
    else
    {
        ui->frameCtrlFreq->setFrequency(_rx_frequency + _demod_offset, false);
        _transmitting_radio=false;
        endTx();
    }
}

void MainWindow::sendTextRequested()
{
    QString text = ui->sendTextEdit->toPlainText();
    emit sendText(text, false);
    ui->sendTextEdit->setPlainText("");
}

void MainWindow::newFFTData(float *fft_data, int fftsize)
{
    // don't paint anything if window is minimized
    if(isMinimized())
        return;

    // FIXME: fftsize is a reference

    if (fftsize == 0)
    {
        /* nothing to do, wait until next activation. */
        return;
    }

    for (int i = 0; i < fftsize; i++)
    {

        // normalize and shift
        if (i < fftsize/2)
        {
            _realFftData[i] = fft_data[fftsize/2+i];
        }
        else
        {
            _realFftData[i] = fft_data[i-fftsize/2];
        }

        // FFT averaging
        _iirFftData[i] += _fft_averaging * (_realFftData[i] - _iirFftData[i]);
    }

    ui->plotterFrame->setNewFftData(_iirFftData, _realFftData, fftsize);
}


void MainWindow::setFFTSize(int size)
{
    emit newFFTSize(ui->fftSizeBox->currentText().toInt());
}

void MainWindow::setAveraging(int x)
{
    _fft_averaging = 1.0 / x;
}

void MainWindow::newWaterfallFPS()
{
    _waterfall_fps = ui->fpsBox->currentText().toInt();
    //ui->plotterFrame->setFftRate(_waterfall_fps);
    emit setWaterfallFPS(_waterfall_fps);
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
    for(int i = 0;i < constellation_data->size();i++)
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
    if(ui->receivedTextEdit->toPlainText().size() > 1024*1024)
    {
        ui->receivedTextEdit->clear();
    }
    if(html)
        ui->receivedTextEdit->insertHtml(text);
    else
        ui->receivedTextEdit->append(text);

    ui->receivedTextEdit->verticalScrollBar()->setValue(ui->receivedTextEdit->verticalScrollBar()->maximum());

    // text widget
    ui->secondaryTextDisplay->moveCursor(QTextCursor::End);
    if(ui->secondaryTextDisplay->toPlainText().size() > 1024*6)
    {
        ui->secondaryTextDisplay->clear();
    }
    if(html)
        ui->secondaryTextDisplay->insertHtml(text);
    else
        ui->secondaryTextDisplay->append(text);

    ui->secondaryTextDisplay->verticalScrollBar()->setValue(ui->secondaryTextDisplay->verticalScrollBar()->maximum());
    ui->secondaryTextDisplay->show();
    _secondary_text_timer.start(10000);
}

void MainWindow::displayVOIPText(QString text, bool html)
{
    ui->voipMessagesEdit->moveCursor(QTextCursor::End);
    if(ui->voipMessagesEdit->toPlainText().size() > 1024*1024)
    {
        ui->voipMessagesEdit->clear();
    }
    if(html)
        ui->voipMessagesEdit->insertHtml(text);
    else
        ui->voipMessagesEdit->append(text);

    ui->voipMessagesEdit->verticalScrollBar()->setValue(ui->voipMessagesEdit->verticalScrollBar()->maximum());
}

void MainWindow::clearTextArea()
{
    ui->receivedTextEdit->setPlainText("");
    ui->receivedTextEdit->moveCursor(QTextCursor::End);
    ui->receivedTextEdit->verticalScrollBar()->setValue(ui->receivedTextEdit->verticalScrollBar()->maximum());
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
    emit connectToServer(ui->voipServerEdit->text(), 64738);
    emit setMute(false);
}

void MainWindow::disconnectVOIPRequested()
{
    emit disconnectFromServer();
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
            if(QString::number(stations.at(i).id) == ".")
                continue;
            QTreeWidgetItem *item = channel_list.at(0);
            QTreeWidgetItem *st_item = new QTreeWidgetItem(0);
            st_item->setText(0,stations.at(i).callsign);
            st_item->setText(3,QString::number(stations.at(i).id));
            st_item->setIcon(0,QIcon(":/res/im-user.png"));
            st_item->setBackgroundColor(0,QColor("#ffffff"));
            st_item->setBackgroundColor(1,QColor("#ffffff"));
            st_item->setBackgroundColor(2,QColor("#ffffff"));
            st_item->setBackgroundColor(3,QColor("#ffffff"));
            if(stations.at(i).is_user)
                st_item->setTextColor(0,QColor("#cc0000"));
            item->addChild(st_item);
            _user_list.append(stations.at(i));
        }
    }
}

void MainWindow::leftStation(Station *s)
{
    QList<QTreeWidgetItem*> list = ui->voipTreeWidget->findItems(QString::number(s->id),
             Qt::MatchExactly | Qt::MatchRecursive,3);
    for(int i =0;i<_user_list.size();i++)
    {
        if(_user_list.at(i).id == s->id)
        {
            _user_list.remove(i);
        }
    }
    if(list.size()>0)
    {
        delete list.at(0);
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
        QList<QTreeWidgetItem*> list = ui->voipTreeWidget->findItems(QString::number(_user_list.at(i).id),
                 Qt::MatchExactly | Qt::MatchRecursive,3);
        if(list.size()>0)
        {
            list.at(0)->setIcon(0, QIcon(":res/im-user.png"));
        }
    }
}

void MainWindow::newChannel(MumbleChannel *chan)
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
            old_item->setIcon(0,QIcon(":/res/call-start.png"));
        }
        QTreeWidgetItem *item = ui->voipTreeWidget->findItems(QString::number(chan->id),Qt::MatchExactly | Qt::MatchRecursive,2).at(0);
        item->setBackgroundColor(0,QColor("#cc0000"));
        item->setBackgroundColor(1,QColor("#cc0000"));
        item->setBackgroundColor(2,QColor("#cc0000"));
        item->setTextColor(0,QColor("#ffffff"));
        item->setTextColor(1,QColor("#ffffff"));
        item->setTextColor(2,QColor("#ffffff"));
        item->setIcon(0,QIcon(":/res/call-start.png"));
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
    t->setIcon(0,QIcon(":/res/call-start.png"));
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
            t->setIcon(0,QIcon(":/res/call-start.png"));
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
    emit setSampleRate(ui->sampleRateBox->currentText().toInt());
    emit toggleRX(value);
    ui->plotterFrame->setRunningState(value);
    setFFTSize(_settings->waterfall_fps);
    newWaterfallFPS();
    _range_set = false;
}

void MainWindow::toggleTXwin(bool value)
{
    emit toggleTX(value);
}

void MainWindow::toggleWideband(bool value)
{
    emit toggleWidebandMode(value);
}

void MainWindow::setFilterWidth(int index)
{
    std::complex<int> widths = _filter_widths->at(index);
    ui->plotterFrame->setHiLowCutFrequencies(widths.real(), widths.imag());
}

void MainWindow::toggleRxMode(int value)
{
    emit toggleRxModemMode(value);
    setFilterWidth(value);
    //mainTabChanged(ui->tabWidget->currentIndex());
}

void MainWindow::toggleTxMode(int value)
{
    emit toggleTxModemMode(value);
    //mainTabChanged(ui->tabWidget->currentIndex());
}

void MainWindow::initError(QString error)
{
    std::cerr << error.toStdString() << std::endl;
    ui->tabWidget->setCurrentIndex(3);
}

void MainWindow::toggleRepeater(bool value)
{
    emit toggleRepeat(value);
}

void MainWindow::clarifierTuneFreq(int value)
{
    emit fineTuneFreq((int)ui->tuneDial->value());
}

void MainWindow::tuneMainFreq(qint64 freq)
{

    ui->frequencyEdit->setText(QString::number(ceil(freq/1000)));
    ui->tuneDial->setValue(0);
    // _rx_frequency is the center frequency of the source
    _rx_frequency = freq - _demod_offset;
    // tx_frequency is the actual frequency
    _tx_frequency = freq;
    ui->plotterFrame->setCenterFreq(_rx_frequency);
    ui->plotterFrame->setDemodCenterFreq(_rx_frequency + _demod_offset);
    emit setCarrierOffset(_demod_offset);
    emit tuneFreq(_rx_frequency);
    emit tuneTxFreq(freq);
}


// this happens on middle mouse
void MainWindow::tuneFreqPlotter(qint64 freq)
{
    return; // can't handle this now
    _tx_frequency = freq;
    tuneMainFreq(freq - _demod_offset);
    ui->frameCtrlFreq->setFrequency(freq + _demod_offset);
}

void MainWindow::carrierOffsetChanged(qint64 freq, qint64 offset)
{
    _demod_offset = offset;
    ui->frameCtrlFreq->setFrequency(_rx_frequency + _demod_offset, false);
    emit setCarrierOffset(offset);
    emit tuneTxFreq(freq);
}

void MainWindow::enterFreq()
{
    ui->frameCtrlFreq->setFrequency(ui->frequencyEdit->text().toLong()*1000);
    _rx_frequency = ui->frequencyEdit->text().toLong()*1000 - _demod_offset;
    emit tuneFreq(_rx_frequency);
}

void MainWindow::enterShift()
{
    if(!_transmitting_radio)
    {
        _tx_shift_frequency = ui->shiftEdit->text().toLong()*1000;
        emit changeTxShift(_tx_shift_frequency);
    }
    else
    {
        std::cerr << "Cannot set TX shift frequency while transmitting" << std::endl;
    }
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




void MainWindow::mainTabChanged(int value)
{

}

void MainWindow::updateFreqGUI(long long center_freq, long carrier_offset)
{
    // Lots of signals flowing around
    _demod_offset = carrier_offset;
    _rx_frequency = (qint64)center_freq;
    ui->frameCtrlFreq->setFrequency(_rx_frequency + _demod_offset, false);
    ui->plotterFrame->setFilterOffset((qint64)_demod_offset);
    ui->plotterFrame->setCenterFreq(_rx_frequency);
    ui->frequencyEdit->setText(QString::number(ceil((_rx_frequency+_demod_offset)/1000)));
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

    float S9 = 80.0; // degrees
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
    _rx_sample_rate = samp_rate;
    ui->plotterFrame->setSampleRate(samp_rate);
    ui->plotterFrame->setSpanFreq((quint32)samp_rate);
    emit setSampleRate(samp_rate);
}

void MainWindow::setFFTRange(int value)
{
    // value is from one to 10
    if(_rssi == 0)
        _rssi = -80.0;
    ui->plotterFrame->setPandapterRange(_rssi - 20 / (float)value , _rssi + 70 / (float)value);
    ui->plotterFrame->setWaterfallRange(_rssi - 20 / (float)value , _rssi + 70 / (float)value);
}

void MainWindow::autoSquelch()
{
    if(_rssi == 0)
        return;
    int squelch = (int)_rssi + 50;
    setSquelchDisplay(squelch);
    ui->rxSquelchDial->setValue(squelch);
}
