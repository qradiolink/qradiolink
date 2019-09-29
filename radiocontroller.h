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
#include <iostream>
#include <unistd.h>
#include <math.h>
#include <gnuradio/digital/crc32.h>
#include <libconfig.h++>
#include <ftdi.h>
#include "audio/audiointerface.h"
#include "settings.h"
#include "radiochannel.h"
#include "mumblechannel.h"
#include "radioprotocol.h"
#include "relaycontroller.h"
#include "station.h"
#include "audio/audioencoder.h"
#include "video/videoencoder.h"
#include "audio/alsaaudio.h"
#include "gr/gr_modem.h"
#include "net/netdevice.h"


typedef QVector<Station> StationList;
typedef std::vector<std::complex<float>> complex_vector;
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
    explicit RadioController(Settings *settings,
                      QObject *parent = 0);
    ~RadioController();

    void flushVoipBuffer();
    void updateDataModemReset(bool transmitting, bool ptt_activated);

signals:
    void finished();
    void setAudioReadMode(bool capture, bool preprocess, int audio_mode);
    void printText(QString text, bool html);
    void printCallsign(QString text);
    void displayReceiveStatus(bool status);
    void displayTransmitStatus(bool status);
    void displayDataReceiveStatus(bool status);
    void audioData(unsigned char *buf, int size);
    void pcmData(std::vector<float> *pcm);
    void videoData(unsigned char *buf, int size);
    void netData(unsigned char *buf, int size);
    void videoImage(QImage img);
    void endAudio(int secs);
    void startAudio();
    void freqToGUI(long long center_freq,long carrier_offset);
    void pingServer();
    void voipDataPCM(short *pcm, int samples);
    void voipDataOpus(unsigned char *pcm, int packet_size);
    void newFFTData(float*, int);
    void newConstellationData(complex_vector*);
    void newRSSIValue(float rssi);
    void initError(QString error);
    void writePCM(short *pcm, int bytes, bool preprocess, int mode);


public slots:
    void run();
    void startTransmission();
    void endTransmission();
    void txAudio(short *audiobuffer, int audiobuffer_size, int vad, bool radio_only);
    void textData(QString text, bool repeat = false);
    void stop();
    void textReceived(QString text);
    void repeaterInfoReceived(QByteArray data);
    void callsignReceived(QString callsign);
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
    void fineTuneFreq(long center_freq);
    void tuneFreq(qint64 center_freq);
    void tuneTxFreq(qint64 actual_freq);
    void changeTxShift(qint64 center_freq);
    void setTxPower(int dbm);
    void setBbGain(int value);
    void setSquelch(int value);
    void setVolume(int value);
    void setRxSensitivity(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void enableGUIConst(bool value);
    void enableGUIFFT(bool value);
    void enableRSSI(bool value);
    void enableDuplex(bool value);
    void scan(bool receiving, bool wait_for_timer=true);
    void startScan(int step, int direction);
    void stopScan();
    void startMemoryScan(RadioChannels *mem, int direction);
    void stopMemoryScan();
    void endAudioTransmission();
    void processVoipAudioFrame(short *pcm, int samples, quint64 sid);
    void usePTTForVOIP(bool value);
    void setVOIPForwarding(bool value);
    void startTx();
    void stopTx();
    void endTx();
    void updateFrequency();
    void toggleRepeat(bool value);
    void addChannel(MumbleChannel* chan);
    void setStations(StationList list);
    void setVox(bool value);
    void setCarrierOffset(qint64 offset);
    void setFFTSize(int size);
    void setFFTPollTime(int fps);
    void setRxSampleRate(int samp_rate);

private:
    void readConfig(std::string &rx_device_args, std::string &tx_device_args,
                    std::string &rx_antenna, std::string &tx_antenna, int &rx_freq_corr,
                    int &tx_freq_corr, std::string &callsign, std::string &video_device);
    int getFrameLength(unsigned char *data);
    unsigned int getFrameCRC32(unsigned char *data);


    void updateInputAudioStream();
    int processInputVideoStream(bool &frame_flag);
    void processInputNetStream();
    void sendEndBeep();
    void sendChannels();
    void sendTextData(QString text, int frame_type);
    void sendBinData(QByteArray data, int frame_type);
    bool getDemodulatorData();
    void getFFTData();
    void getConstellationData();
    void getRSSI();
    void setRelays(bool transmitting);
    void memoryScan(bool receiving, bool wait_for_timer=true);

    // FIXME: inflation of members
    AudioInterface *_audio;
    Settings *_settings;
    RelayController *_relay_controller;
    AudioEncoder *_codec;
    VideoEncoder *_video;
    NetDevice *_net_device;
    gr_modem *_modem;
    RadioProtocol *_radio_protocol;
    QMutex *_mutex;
    QTimer *_voice_led_timer;
    QTimer *_data_led_timer;
    QTimer *_vox_timer;
    QTimer *_voip_tx_timer;
    QTimer *_end_tx_timer;
    QElapsedTimer *_data_read_timer;
    QElapsedTimer *_data_modem_reset_timer;
    QElapsedTimer *_data_modem_sleep_timer;
    QElapsedTimer *_fft_read_timer;
    QElapsedTimer *_const_read_timer;
    QElapsedTimer *_rssi_read_timer;
    QElapsedTimer *_scan_timer;
    std::vector<short> *_m_queue;
    unsigned char *_rand_frame_data;
    float *_fft_data;
    QVector<short> *_voip_encode_buffer;
    QByteArray *_data_rec_sound;
    QByteArray *_end_rec_sound;

    bool _stop;
    bool _tx_inited;
    bool _rx_inited;
    bool _voip_enabled;
    bool _voip_forwarding;
#if 0
    AlsaAudio *_audio;
#endif

    bool _transmitting_audio;
    bool _process_text;
    bool _repeat_text;
    QString _text_out;
    QString _callsign;


    int _rx_mode;
    int _tx_mode;
    int _rx_radio_type;
    int _tx_radio_type;
    long long _rx_frequency;
    long long _tx_frequency;
    long long _autotune_freq;
    long long _tune_shift_freq;
    float _tx_power;
    int _bb_gain;
    int _squelch;
    double _rx_sensitivity;
    int _step_hz;
    int _scan_step_hz;
    int _tune_limit_lower;
    int _tune_limit_upper;
    bool _scan_done;
    int _memory_scan_index;
    bool _memory_scan_done;
    bool _tx_modem_started;
    int _tune_counter;
    float _rx_ctcss;
    float _tx_ctcss;
    float _rx_volume;
    long long _rx_sample_rate;
    QElapsedTimer _last_voiced_frame_timer;
    bool _data_modem_sleeping;
    quint64 _last_session_id;
    bool _repeat;
    bool _vox_enabled;
    bool _tx_started;
    int _freq_gui_counter;
    qint64 _carrier_offset;
    bool _fft_enabled;
    int _fft_poll_time;
    bool _constellation_enabled;
    bool _rssi_enabled;
    bool _duplex_enabled;
    bool _scan_stop;
    QList<radiochannel*> _memory_channels;

};

#endif // RADIOOP_H
