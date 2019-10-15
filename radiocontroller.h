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
#include "audio/audioprocessor.h"
#include "settings.h"
#include "radiochannel.h"
#include "mumblechannel.h"
#include "radioprotocol.h"
#include "relaycontroller.h"
#include "station.h"
#include "audio/audioencoder.h"
#include "video/videoencoder.h"
#include "gr/gr_modem.h"
#include "net/netdevice.h"


typedef QVector<Station*> StationList;
typedef std::vector<std::complex<float>> complex_vector;
typedef std::vector<std::string> string_vector;
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

    void flushRadioToVoipBuffer();
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
    void rxGainStages(string_vector rx_gains);
    void txGainStages(string_vector tx_gains);


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
    void changeTxShift(qint64 shift_freq);
    void setTxPower(int dbm, std::string gain_stage="");
    void setRxSensitivity(int value, std::string gain_stage="");
    void setBbGain(int value);
    void setAgcAttack(float value);
    void setAgcDecay(float value);
    void setSquelch(int value);
    void setVolume(int value);
    void setTxVolume(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void setFilterWidth(int width);
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
    void stopVoipTx();
    void toggleRepeat(bool value);
    void setChannels(ChannelList channels);
    void setStations(StationList list);
    void setVox(bool value);
    void setCarrierOffset(qint64 offset);
    void setFFTSize(int size);
    void setFFTPollTime(int fps);
    void setRxSampleRate(int samp_rate);
    void enableAudioCompressor(bool value);
    void enableRelays(bool value);
    void calibrateRSSI(float value);
    void setCallsign();

private:
    unsigned int getFrameLength(unsigned char *data);
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
    void processVoipToRadioQueue();


    // FIXME: inflation of members
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
    QVector<short> *_to_radio_queue;
    unsigned char *_rand_frame_data;
    float *_fft_data;
    QVector<short> *_to_voip_buffer;
    QByteArray *_data_rec_sound;
    QByteArray *_end_rec_sound;
    QList<radiochannel*> _memory_channels;

    QString _text_out;
    QString _callsign;
    QElapsedTimer _last_voiced_frame_timer;

    bool _stop;
    bool _transmitting;
    bool _process_text;
    bool _repeat_text;
    bool _scan_done;
    bool _scan_stop;
    bool _memory_scan_done;
    bool _tx_modem_started;
    bool _data_modem_sleeping;

    int _rx_mode;
    int _tx_mode;
    int _rx_radio_type;
    int _tx_radio_type;
    long long _tx_frequency;
    long long _autotune_freq;
    int _step_hz;
    int _scan_step_hz;
    int _tune_limit_lower;
    int _tune_limit_upper;
    int _memory_scan_index;
    float _rx_volume;
    float _tx_volume;
    int _fft_poll_time;
    quint64 _last_session_id;

};

#endif // RADIOOP_H
