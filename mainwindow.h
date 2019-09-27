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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QScrollBar>
#include <QDir>
#include <QFileInfo>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPainter>
#include <QToolTip>
#include <math.h>
#include <complex>
#include <libconfig.h++>
#include "ext/utils.h"
#include "mumbleclient.h"
#include "radiochannel.h"
#include "qtgui/freqctrl.h"
#include "qtgui/plotter.h"
#include "settings.h"
#include "mumblechannel.h"

#include <iostream>

typedef std::vector<std::complex<float>> complex_vector;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public slots:
    void startTx();
    void endTx();
    void connectVOIPRequested();
    void disconnectVOIPRequested();
    void connectedToServer(QString msg);
    void resetSpeechIcons();
    void sendTextRequested();
    void chooseFile();
    void displayText(QString text, bool html);
    void displayVOIPText(QString text, bool html);
    void displayCallsign(QString callsign);
    void displayReceiveStatus(bool status);
    void displayTransmitStatus(bool status);
    void displayDataReceiveStatus(bool status);
    void updateOnlineStations(StationList stations);
    void leftStation(Station *s);
    void userSpeaking(quint64 id);
    void toggleRXwin(bool value);
    void toggleTXwin(bool value);
    void clarifierTuneFreq(int value);
    void tuneMainFreq(qint64 freq);
    void tuneFreqPlotter(qint64 freq);
    void toggleWideband(bool value);
    void toggleRxMode(int value);
    void toggleTxMode(int value);
    void setTxPowerDisplay(int value);
    void setRxSensitivityDisplay(int value);
    void setSquelchDisplay(int value);
    void setVolumeDisplay(int value);
    void startScan(bool value);
    void displayImage(QImage img);
    void enterFreq();
    void saveConfig();
    void updateMemories();
    void mainTabChanged(int value);
    void clearTextArea();
    void updateFreqGUI(long long center_freq, long carrier_offset);
    void enterShift();
    void updateRxCTCSS(int value);
    void updateTxCTCSS(int value);
    void togglePTTVOIP(bool value);
    void toggleVOIPForwarding(bool value);
    void toggleVox(bool value);
    void toggleRepeater(bool value);
    void newChannel(MumbleChannel *chan);
    void channelState(QTreeWidgetItem *item, int k);
    void newFFTData(float* fft_data, int fftsize);
    void carrierOffsetChanged(qint64 freq, qint64 offset);
    void setFFTSize(int size);
    void setAveraging(int x);
    void showControls(bool value);
    void showConstellation(bool value);
    void setEnabledFFT(bool value);
    void setEnabledDuplex(bool value);
    void setPeakDetect(bool value);
    void updateRSSI(float value);
    void updateConstellation(complex_vector* constellation_data);
    void newWaterfallFPS();
    void updateSampleRate();
    void setFFTRange(int value);
    void autoSquelch();
    void initError(QString error);
    void showMemoriesPanel(bool show);
    void addMemoryChannel();
    void removeMemoryChannel();
    void tuneToMemoryChannel(int row, int col);
    void editMemoryChannel(QTableWidgetItem* item);
    void startMemoryScan(bool value);


signals:
    void startTransmission();
    void endTransmission();
    void sendText(QString text, bool repeat);
    void toggleRX(bool value);
    void toggleTX(bool value);
    void tuneFreq(qint64 center_freq);
    void tuneTxFreq(qint64 freq);
    void changeTxShift(qint64 center_freq);
    void fineTuneFreq(long center_freq);
    void toggleWidebandMode(bool value);
    void toggleRxModemMode(int value);
    void toggleTxModemMode(int value);
    void setTxPower(int value);
    void setRxSensitivity(int value);
    void setSquelch(int value);
    void setVolume(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void enableGUIConst(bool value);
    void enableGUIFFT(bool value);
    void enableDuplex(bool value);
    void enableRSSI(bool value);
    void startAutoTuneFreq(int step, int scan_direction);
    void stopAutoTuneFreq();
    void startMemoryTune(RadioChannels* channels, int scan_direction);
    void stopMemoryTune();
    void usePTTForVOIP(bool value);
    void setVOIPForwarding(bool value);
    void setVox(bool value);
    void connectToServer(QString server, unsigned port);
    void disconnectFromServer();
    void changeChannel(int id);
    void setMute(bool value);
    void toggleRepeat(bool value);
    void stopRadio();
    void setCarrierOffset(qint64 offset);
    void newFFTSize(int);
    void setWaterfallFPS(int);
    void setSampleRate(int);


public:
    explicit MainWindow(Settings *settings, RadioChannels *radio_channels, QWidget *parent = 0);
    ~MainWindow();

    void readConfig();
    void initSettings();

private:
    void addDisplayChannel(radiochannel *chan, int r);
    void closeEvent(QCloseEvent *);
    void changeEvent(QEvent *);
    void resizeEvent(QResizeEvent *);
    QFileInfo *setupConfig();
    void setFilterWidth(int index);

    // FIXME: inflation of members
    Ui::MainWindow *ui;
    Settings *_settings;
    RadioChannels *_radio_channels;
    QPixmap *_video_img;
    QPixmap *_constellation_img;
    QFileInfo *_config_file;
    QPainter *_constellation_painter;
    QPixmap *_s_meter_bg;
    QGraphicsOpacityEffect *_eff_freq;
    QGraphicsOpacityEffect *_eff_const;
    QGraphicsOpacityEffect *_eff_video;
    QGraphicsOpacityEffect *_eff_text_display;
    std::vector<std::complex<int>> *_filter_widths;
    float *_realFftData;
    float *_iirFftData;

    StationList _user_list;
    bool _transmitting_radio;
    qint64 _rx_frequency;
    qint64 _tx_frequency;
    qint64 _tx_shift_frequency;
    int _rx_mode;
    int _tx_mode;
    int _current_voip_channel;

    float _fft_averaging;
    int _waterfall_fps;
    long _rx_sample_rate;
    qint64 _demod_offset;
    float _rssi;
    bool _range_set;
    QTimer _secondary_text_timer;
    QTimer _video_timer;
    QTimer _speech_icon_timer;
    QMutex _mutex;
    int _new_mem_index;

};

#endif // MAINWINDOW_H
