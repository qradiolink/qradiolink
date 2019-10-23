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
#include <QAudioDeviceInfo>
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


typedef std::vector<std::complex<float>> complex_vector;
typedef QMap<std::string,QVector<int>> gain_vector;
typedef QVector<MumbleChannel*> ChannelList;
typedef QVector<Station*> StationList;


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
    void disconnectedFromServer();
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
    void updateChannels(ChannelList channels);
    void joinedChannel(quint64 channel_id);
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
    void setTxVolumeDisplay(int value);
    void startScan(bool value);
    void displayImage(QImage img);
    void enterFreq();
    void saveUiConfig();
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
    void tuneToMemoryChannel(radiochannel *chan);
    void editMemoryChannel(QTableWidgetItem* item);
    void saveMemoryChannes();
    void startMemoryScan(bool value);
    void changeFilterWidth(int low, int up);
    void setAudioCompressor(bool value);
    void setRelays(bool value);
    void setRemoteControl(bool value);
    void setRSSICalibration();
    void setDigitalGain(int value);
    void updateAgcAttack(int value);
    void updateAgcDecay(int value);
    void setRxGainStages(gain_vector rx_gains);
    void setTxGainStages(gain_vector tx_gains);
    void sendMumbleTextMessage();
    void toggleSelfDeaf(bool deaf);
    void toggleSelfMute(bool mute);
    void changeVoipVolume(int value);
    void setBurstIPMode(bool value);
    void setTheme(bool value);
    void updateScanResumeTime(int value);

signals:
    void terminateConnections();
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
    void setTxVolume(int value);
    void setVoipVolume(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void enableGUIConst(bool value);
    void enableGUIFFT(bool value);
    void enableDuplex(bool value);
    void enableRSSI(bool value);
    void startAutoTuneFreq(int step, int scan_direction);
    void stopAutoTuneFreq();
    void startMemoryTune(int scan_direction);
    void stopMemoryTune();
    void usePTTForVOIP(bool value);
    void setVOIPForwarding(bool value);
    void setVox(bool value);
    void connectToServer(QString server, unsigned port);
    void disconnectFromServer();
    void setSelfDeaf(bool deaf, bool mute);
    void setSelfMute(bool mute);
    void changeChannel(int id);
    void setMute(bool value);
    void toggleRepeat(bool value);
    void stopRadio();
    void setCarrierOffset(qint64 offset);
    void newFFTSize(int);
    void setWaterfallFPS(int);
    void setSampleRate(int);
    void newFilterWidth(int);
    void enableAudioCompressor(bool value);
    void enableRelays(bool value);
    void calibrateRSSI(float value);
    void setBbGain(int value);
    void setAgcAttack(float value);
    void setAgcDecay(float value);
    void newMumbleMessage(QString text);
    void enableRemote();
    void disableRemote();
    void setScanResumeTime(int value);


public:
    explicit MainWindow(Settings *settings, RadioChannels *radio_channels, QWidget *parent = 0);
    ~MainWindow();

    void setConfig();
    void initSettings();

private:
    void addDisplayChannel(radiochannel *chan, int r);
    void closeEvent(QCloseEvent *);
    void changeEvent(QEvent *);
    void resizeEvent(QResizeEvent *);
    QFileInfo *setupConfig();
    void setFilterWidth(int index);

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
    QGraphicsOpacityEffect *_eff_mem_display;
    std::vector<std::complex<int>> *_filter_widths;
    std::vector<std::complex<int>> *_filter_ranges;
    std::vector<bool> *_filter_symmetric;
    QVector<QString> *_mode_list;
    float *_realFftData;
    float *_iirFftData;

    QTimer _secondary_text_timer;
    QTimer _video_timer;
    QTimer _speech_icon_timer;
    QMutex _mutex;
    float _rssi;
    bool _ptt_activated;
    int _current_voip_channel;

    StationList _user_list;

    bool _range_set;
    int _new_mem_index;
    int _filter_low_cut;
    int _filter_high_cut;
    bool _filter_is_symmetric;

    QList<QAudioDeviceInfo> _audio_output_devices;
    QList<QAudioDeviceInfo> _audio_input_devices;

    QVector<QSlider*> _rx_gain_sliders;
    QVector<QSlider*> _tx_gain_sliders;

};

#endif // MAINWINDOW_H
