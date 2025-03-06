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

#ifndef RADIOOP_H
#define RADIOOP_H

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QMutex>
#include <QDir>
#include <QByteArray>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QtConcurrent/QtConcurrent>
#include <unistd.h>
#include <cmath>
#include "ext/crc32.h"
#include "settings.h"
#include "radiochannel.h"
#include "mumblechannel.h"
#include "layer2.h"
#include "radiochannel.h"
#include "relaycontroller.h"
#include "limerfecontroller.h"
#include "station.h"
#include "audio/audiomixer.h"
#include "audio/audioencoder.h"
#include "video/videoencoder.h"
#include "video/imagecapture.h"
#include "src/gr_modem.h"
#include "src/DMR/dmrcontrol.h"
#include "net/netdevice.h"
#include "logger.h"
#include "src/config_mmdvm.h"


typedef QVector<Station*> StationList;
typedef std::vector<std::complex<float>> complex_vector;
typedef QMap<std::string,QVector<int>> gain_vector;
namespace radio_type
{
    enum
    {
        RADIO_TYPE_DIGITAL,
        RADIO_TYPE_ANALOG
    };
}

class RadioController : public QObject
{
    Q_OBJECT
public:
    explicit RadioController(Settings *settings, Logger *logger, RadioChannels *radio_channels,
                      QObject *parent = 0);
    ~RadioController();

    void flushRadioToVoipBuffer();
    void updateDataModemReset(bool transmitting, bool ptt_activated);
    void callbackOnReceive();


signals:
    void finished();
    void setAudioReadMode(bool capture, bool preprocess, int audio_mode, int audiobuffer_size);
    void printText(QString text, bool html);
    void printCallsign(QString text);
    void displayReceiveStatus(bool status);
    void displayTransmitStatus(bool status);
    void displayDataReceiveStatus(bool status);
    void audioData(unsigned char *buf, int size);
    void m17AudioData(unsigned char *buf, int size);
    void dmrAudioData(unsigned char *buf, int size);
    void pcmData(std::vector<float> *pcm);
    void videoData(unsigned char *buf, int size);
    void netData(unsigned char *buf, int size);
    void voipVideoData(unsigned char *buf, int size);
    void videoImage(QImage img);
    void endAudio(int secs);
    void startAudio();
    void freqToGUI(int64_t center_freq, int64_t carrier_offset);
    void clarifierFreqToGUI(int clarifier_offset);
    void voipDataPCM(short *pcm, int samples);
    void voipDataOpus(unsigned char *pcm, int packet_size);
    void newFFTData(float*, int);
    void newSampleData(float*, int);
    void newConstellationData(complex_vector*);
    void newRSSIValue(float rssi);
    void initError(QString error, int index);
    void writePCM(short *pcm, int bytes, bool preprocess, int mode);
    void udpAudioSamples(short *pcm, int samples);
    void rxGainStages(gain_vector rx_gains);
    void txGainStages(gain_vector tx_gains);
    void setSelfDeaf(bool deaf);
    void setSelfMute(bool mute);
    void newMumbleMessage(QString text);
    void tuneToMemoryChannel(radiochannel *chan);
    void recordAudio(bool value);
    void newPageMessage(QString paged_by, QString message);
    void terminateConnections();
    void enableUDPAudio(bool value);
    void startReceiveTimer(int value);


public slots:
    void run();
    void startTransmission();
    void endTransmission();
    void radioTimeout();
    void txAudio(short *audiobuffer, int audiobuffer_size, int vad, bool radio_only);
    void processVideoFrame(unsigned char *audio_buffer, int audio_size);
    void textData(QString text, bool repeat = false);
    void textMumble(QString text, bool channel = false);
    void stop();
    void textReceived(QString text, bool html);
    void callsignReceived(QString callsign);
    void m17FrameInfoReceived(QString src, QString dest, uint16_t CAN);
    void protoReceived(QByteArray data);
    void audioFrameReceived();
    void dataFrameReceived();
    void receiveEnd();
    void receiveDigitalAudio(unsigned char *data, int size);
    void receiveVideoData(unsigned char *data, int size);
    void receiveNetData(unsigned char *data, int size);
    void receivePCMAudio(std::vector<float>* audio_data);
    void toggleRX(bool value);
    void toggleTX(bool value);
    void toggleRxMode(int value);
    void toggleTxMode(int value);
    void fineTuneFreq(qint64 center_freq);
    void tuneFreq(qint64 center_freq);
    void tuneTxFreq(qint64 actual_freq);
    void changeTxShift(qint64 shift_freq);
    void setTxPower(int value, std::string gain_stage="");
    void setRxSensitivity(int value, std::string gain_stage="");
    void setBbGain(int value);
    void setIfGain(int value);
    void setAgcAttack(int value);
    void setAgcDecay(int value);
    void setSquelch(int value);
    void setVolume(int value);
    void setTxVolume(int value);
    void setVoipVolume(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void setFilterWidth(int width);
    void setLimeRFEAttenuation(int value);
    void enableGUIConst(bool value);
    void enableGUIFFT(bool value);
    void enableTimeDomain(bool value);
    void enableRSSI(bool value);
    void enableDuplex(bool value);
    void scan(bool receiving, bool wait_for_timer=true);
    void startScan(int step, int direction);
    void stopScan();
    void startMemoryScan(int direction);
    void stopMemoryScan();
    void tuneMemoryChannel(radiochannel *chan);
    void endAudioTransmission();
    void endBeep();
    void processVoipAudioFrame(short *pcm, int samples, quint64 sid);
    void processVoipVideoFrame(unsigned char *video_frame, int size, quint64 sid);
    void usePTTForVOIP(bool value);
    void setVOIPForwarding(bool value);
    void setUDPAudio(bool value);
    void setUDPAudioSampleRate(int value);
    void startTx();
    void stopTx();
    void endTx();
    void stopVoipTx();
    void callbackStopReceive();
    void toggleRepeat(bool value);
    void setChannels(ChannelList channels);
    void setStations(StationList list);
    void setVox(bool value);
    void setCarrierOffset(qint64 offset);
    void setTxCarrierOffset(qint64 offset);
    void resetTxCarrierOffset();
    void setFFTSize(int size);
    void setFFTPollTime(int fps);
    void setSampleWindow(uint size);
    void setTimeDomainSampleRate(int samp_rate);
    void setTimeDomainFilterWidth(int filter_width);
    void setRxSampleRate(int samp_rate);
    void enableAudioCompressor(bool value);
    void enableRelays(bool value);
    void enableLimeRFE(bool value);
    void setLimeRFENotch(bool value);
    void calibrateRSSI(float value);
    void setCallsign();
    void setScanResumeTime(int value);
    void setAudioRecord(bool value);
    void setVoxLevel(int value);
    void setVoipBitrate(int value);
    void setEndBeep(int value);
    void enableReverseShift(bool value);
    void setMuteForwardedAudio(bool value);
    void setBlockBufferSize(int value);
    void setRadioToT(int value);
    void setTotTxEnd(bool value);
    void setTxLimits(bool value);
    void pageUser(QString user, QString message);
    void receivedPageMessage(QString calling_user, QString called_user, QString page_message);

private:
    u_int32_t getFrameLength(unsigned char *data);
    u_int32_t getFrameCRC32(unsigned char *data);


    void updateInputAudioStream();
    void processInputNetStream();
    void sendTxBeep(int sound=0);
    void transmitServerInfoBeacon();
    void transmitTextData();
    void transmitBinData();
    bool getDemodulatorData();
    void getFFTData();
    void getSampleData();
    void getConstellationData();
    void getRSSI();
    void setRelays(bool transmitting);
    void memoryScan(bool receiving, bool wait_for_timer=true);
    bool processMixerQueue();
    void updateCWK();


    // FIXME: inflation of members
    Settings *_settings;
    Logger *_logger;
    RadioChannels *_radio_channels;
    RelayController *_relay_controller;
    LimeRFEController *_lime_rfe_controller;
    AudioEncoder *_codec;
    AudioMixer *_audio_mixer_in;
    AudioMixer *_audio_mixer_out;
    VideoEncoder *_video;
    ImageCapture *_camera;
    NetDevice *_net_device;
    gr_modem *_modem;
    DMRControl *_dmr_control;
    Layer2Protocol *_layer2;
    QMutex *_mutex;
    QTimer *_voice_led_timer;
    QTimer *_data_led_timer;
    QTimer *_vox_timer;
    QTimer *_voip_tx_timer;
    QTimer *_rx_timer;
    QTimer *_end_tx_timer;
    QTimer *_radio_time_out_timer;
    QElapsedTimer *_data_read_timer;
    QElapsedTimer *_data_modem_reset_timer;
    QElapsedTimer *_data_modem_sleep_timer;
    QElapsedTimer *_fft_read_timer;
    QElapsedTimer *_const_read_timer;
    QElapsedTimer *_rssi_read_timer;
    QElapsedTimer *_scan_timer;
    QElapsedTimer *_cw_timer;
    unsigned char *_rand_frame_data;
    float *_fft_data;
    float *_sample_data;
    QVector<short> *_to_voip_buffer;
    QByteArray *_data_rec_sound;
    QByteArray *_end_rec_sound;
    QByteArray * _timeout_sound;
    QList<radiochannel*> _memory_channels;

    QString _text_out;
    QByteArray _proto_out;
    QString _callsign;
    QString _incoming_text_buffer;
    QByteArray _incoming_proto_buffer;
    QMap<std::string, int> _rx_stage_gains;
    QMap<std::string, int> _tx_stage_gains;

    bool _stop_thread;
    bool _transmitting;
    bool _receiving;
    bool _process_text;
    bool _process_data;
    bool _repeat_text;
    bool _scan_done;
    bool _scan_stop;
    bool _memory_scan_done;
    bool _data_modem_sleeping;
    bool _radio_to_voip_on;
    bool _text_transmit_on;
    bool _proto_transmit_on;
    bool _cw_tone;
    bool _enable_rssi;

    int _rx_mode;
    int _tx_mode;
    int _rx_radio_type;
    int _tx_radio_type;
    int64_t _tx_frequency;
    int64_t _autotune_freq;
    int _step_hz;
    int _scan_step_hz;
    int _tune_limit_lower;
    int _tune_limit_upper;
    int _memory_scan_index;
    float _rx_volume;
    float _tx_volume;
    float _voip_volume;
    int _fft_poll_time;

};

#endif // RADIOOP_H
