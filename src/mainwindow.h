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
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPainter>
#include <QToolTip>
#include <QShortcut>
#include <QAudioDeviceInfo>
#include <QtConcurrent/QtConcurrent>
#include <math.h>
#include <complex>
#include <libconfig.h++>
#include "src/ext/utils.h"
#include "src/mumbleclient.h"
#include "src/radiochannel.h"
#include "src/qtgui/freqctrl.h"
#include "src/qtgui/plotter.h"
#include "src/settings.h"
#include "src/logger.h"
#include "src/mumblechannel.h"
#include "src/style.h"
#include "src/qtgui/skinneddial.h"
#include "ext/devices.h"


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
    void displayAudioLevel(float level);
    void applicationLog(QString msg);
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
    void updateFreqGUI(int64_t center_freq, int64_t carrier_offset);
    void updateClarifierFreqGUI(int clarifier_offset);
    void enterShift();
    void setShiftFromTxFreq(qint64 tx_freq);
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
    void initError(QString error, int index);
    void showMemoriesPanel(bool show);
    void addMemoryChannel();
    void removeMemoryChannel();
    void tuneToMemoryChannel(int row, int col);
    void tuneToMemoryChannel(radiochannel *chan);
    void editMemoryChannel(QTableWidgetItem* item);
    void saveMemoryChannels();
    void startMemoryScan(bool value);
    void changeFilterWidth(qint64 low, qint64 up);
    void setAudioCompressor(bool value);
    void setRelays(bool value);
    void setLimeRFE(bool value);
    void toggleLimeRFENotch(bool value);
    void setRemoteControl(bool value);
    void setRSSICalibration();
    void setDigitalGain(int value);
    void setRxDigitalGain(int value);
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
    void setFFTHistory(bool value);
    void setColouredFFT(bool value);
    void setWaterfallAveraging(bool value);
    void setDrawConstellationEye(bool value);
    void updateScanResumeTime(int value);
    void updateMMDVMChannels(int value);
    void updateMMDVMChannelSeparation(int value);
    void updateAudioOutput(int value);
    void updateAudioInput(int value);
    void toggleAudioRecord(bool value);
    void changeVoxLevel(int value);
    void toggleReverseShift(bool value);
    void updateVoipBitrate(int value);
    void updateEndBeep(int value);
    void updateMuteForwardedAudio(bool value);
    void updateBlockBufferSize(int value);
    void updateTotTimer(int value);
    void updateTotTxEnd(bool value);
    void updateTxLimits(bool value);
    void updateLimeRFEAttenuation(int value);
    void calculateShiftFromTxFreqField();
    void setRxStageGain(int value);
    void setTxStageGain(int value);
    void pageUserRequested();
    void displayPageMessage(QString page_user, QString page_message);
    void updatePanadapterRange(float min, float max);
    void makeFullScreen();
    void updateMemoryChannel();
    void clearMemoryChannel();
    void setGPredictControl(bool value);
    void updateGUIFreq(qint64 freq);
    void tuneDopplerRxFreq(qint64 freq_delta);
    void tuneDopplerTxFreq(qint64 freq_delta);
    void findDevices();
    void updateRXDevices(QString dev_string);
    void updateTXDevices(QString dev_string);

signals:
    void startTransmission();
    void endTransmission();
    void sendText(QString text, bool repeat);
    void toggleRX(bool value);
    void toggleTX(bool value);
    void tuneFreq(qint64 center_freq);
    void tuneTxFreq(qint64 freq);
    void changeTxShift(qint64 center_freq);
    void fineTuneFreq(qint64 center_freq);
    void toggleWidebandMode(bool value);
    void toggleRxModemMode(int value);
    void toggleTxModemMode(int value);
    void setTxPower(int value, std::string gain_stage);
    void setRxSensitivity(int value, std::string gain_stage);
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
    void enableLimeRFE(bool value);
    void calibrateRSSI(float value);
    void setBbGain(int value);
    void setIfGain(int value);
    void setLimeRFEAttenuation(int value);
    void setLimeRFENotch(bool value);
    void setAgcAttack(int value);
    void setAgcDecay(int value);
    void newMumbleMessage(QString text);
    void enableRemote();
    void disableRemote();
    void setScanResumeTime(int value);
    void restartAudioOutputThread();
    void restartAudioInputThread();
    void setAudioRecord(bool value);
    void enableReverseShift(bool value);
    void setVoipBitrate(int value);
    void setEndBeep(int value);
    void setMuteForwardedAudio(bool value);
    void setBlockBufferSize(int value);
    void setRadioToT(int value);
    void setTotTxEnd(bool value);
    void setTxLimits(bool value);
    void pageUser(QString user, QString text);
    void setTxCarrierOffset(qint64 offset);

public:
    explicit MainWindow(Settings *settings, Logger *logger, RadioChannels *radio_channels, QWidget *parent = 0);
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
    Logger *_logger;
    RadioChannels *_radio_channels;
    QPixmap *_video_img;
    QPixmap *_constellation_img;
    QPixmap *_vu_meter_img;
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
    QShortcut *_full_screen_shortcut;
    float *_realFftData;
    float *_iirFftData;

    QTimer _secondary_text_timer;
    QTimer _video_timer;
    QTimer _speech_icon_timer;
    QMutex _mutex;
    float _rssi;
    bool _ptt_activated;
    bool _fft_active;
    bool _controls_active;
    int _current_voip_channel;
    qint64 _tx_frequency;

    StationList _user_list;

    bool _range_set;
    int _new_mem_index;
    int _filter_low_cut;
    int _filter_high_cut;
    bool _filter_is_symmetric;

    QList<QAudioDeviceInfo> _audio_output_devices;
    QList<QAudioDeviceInfo> _audio_input_devices;

    QVector<SkinnedDial*> _rx_gain_sliders;
    QVector<SkinnedDial*> _tx_gain_sliders;
    QMap<std::string, int> _rx_stage_gains;
    QMap<std::string, int> _tx_stage_gains;

};

#endif // MAINWINDOW_H
