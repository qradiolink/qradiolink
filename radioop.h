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
#include <unistd.h>
#include <math.h>
#include "audio/audiointerface.h"
#include "ext/agc.h"
#include "ext/vox.h"
#include "settings.h"
#include "channel.h"
#include "radioprotocol.h"
#include "station.h"
#include "audio/audioencoder.h"
#include "video/videoencoder.h"
#include "audio/alsaaudio.h"
#include "gr/gr_modem.h"
#include "net/netdevice.h"
#include <gnuradio/qtgui/const_sink_c.h>
#include <gnuradio/qtgui/sink_c.h>
#include <gnuradio/qtgui/number_sink.h>
#include <gnuradio/digital/crc32.h>
#include <libconfig.h++>

typedef QVector<Station> StationList;
namespace radio_type
{
    enum
    {
        RADIO_TYPE_DIGITAL,
        RADIO_TYPE_ANALOG
    };
}

class RadioOp : public QObject
{
    Q_OBJECT
public:
    explicit RadioOp(Settings *settings, gr::qtgui::sink_c::sptr fft_gui,
                     gr::qtgui::const_sink_c::sptr const_gui, gr::qtgui::number_sink::sptr rssi_gui, QObject *parent = 0);
    ~RadioOp();

    void flushVoipBuffer();
    void updateDataModemReset(bool transmitting, bool ptt_activated);
signals:
    void finished();
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
    void freqFromGUI(long freq);
    void pingServer();
    void voipData(short *pcm, int samples);
public slots:
    void run();
    void startTransmission();
    void endTransmission();
    void textData(QString text, bool repeat = false);
    void stop();
    void textReceived(QString text);
    void repeaterInfoReceived(QByteArray data);
    void callsignReceived(QString callsign);
    void audioFrameReceived();
    void dataFrameReceived();
    void receiveEnd();
    void receiveAudioData(unsigned char *data, int size);
    void receiveVideoData(unsigned char *data, int size);
    void receiveNetData(unsigned char *data, int size);
    void receivePCMAudio(std::vector<float>* audio_data);
    void toggleRX(bool value);
    void toggleTX(bool value);
    void toggleRxMode(int value);
    void toggleTxMode(int value);
    void fineTuneFreq(long center_freq);
    void tuneFreq(qint64 center_freq);
    void tuneTxFreq(qint64 center_freq);
    void setTxPower(int dbm);
    void setBbGain(int value);
    void setSquelch(int value);
    void setVolume(int value);
    void setRxSensitivity(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void enableGUIConst(bool value);
    void enableGUIFFT(bool value);
    void autoTune();
    void startAutoTune();
    void stopAutoTune();
    void endAudioTransmission();
    void processVoipAudioFrame(short *pcm, int samples, quint64 sid);
    void usePTTForVOIP(bool value);
    void setVOIPForwarding(bool value);
    void startTx();
    void stopTx();
    void updateFrequency();
    void toggleRepeat(bool value);
    void addChannel(Channel* chan);
    void setStations(StationList list);
    void setVox(bool value);

private:
    bool _stop;
    bool _tx_inited;
    bool _rx_inited;
    bool _voip_enabled;
    bool _voip_forwarding;
#if 0
    AlsaAudio *_audio;
#endif
    AudioInterface *_audio;
    Settings *_settings;
    bool _transmitting_audio;
    bool _process_text;
    bool _repeat_text;
    QString _text_out;
    QString _callsign;
    QMutex *_mutex;
    QTimer *_voice_led_timer;
    QTimer *_data_led_timer;
    QTimer *_vox_timer;
    AudioEncoder *_codec;
    VideoEncoder *_video;
    NetDevice *_net_device;
    gr_modem *_modem;
    RadioProtocol *_radio_protocol;
    int _rx_mode;
    int _tx_mode;
    int _rx_radio_type;
    int _tx_radio_type;
    long long _tune_center_freq;
    long long _autotune_freq;
    long long _tune_shift_freq;
    float _tx_power;
    int _bb_gain;
    int _squelch;
    float _rx_sensitivity;
    int _step_hz;
    int _tune_limit_lower;
    int _tune_limit_upper;
    bool _tuning_done;
    bool _tx_modem_started;
    int _tune_counter;
    float _rx_ctcss;
    float _tx_ctcss;
    float _rx_volume;
    QElapsedTimer _last_voiced_frame_timer;
    QTimer *_voip_tx_timer;
    QElapsedTimer *_data_read_timer;
    QElapsedTimer *_data_modem_reset_timer;
    QElapsedTimer *_data_modem_sleep_timer;
    bool _data_modem_sleeping;
    gr::qtgui::sink_c::sptr _fft_gui;
    unsigned char *_rand_frame_data;
    std::vector<short> *_m_queue;
    quint64 _last_session_id;
    QVector<short> *_voip_encode_buffer;
    QByteArray *_data_rec_sound;
    bool _repeat;
    bool _vox_enabled;
    bool _tx_started;
    int _freq_gui_counter;

    void readConfig(std::string &rx_device_args, std::string &tx_device_args,
                    std::string &rx_antenna, std::string &tx_antenna, int &rx_freq_corr,
                    int &tx_freq_corr, std::string &callsign, std::string &video_device);
    int getFrameLength(unsigned char *data);
    void txAudio(short *audiobuffer, int audiobuffer_size);

    void processAudioStream();
    int processVideoStream(bool &frame_flag);
    void processNetStream();
    void sendEndBeep();
    void sendChannels();
    void sendTextData(QString text, int frame_type);
    void sendBinData(QByteArray data, int frame_type);

};

#endif // RADIOOP_H
