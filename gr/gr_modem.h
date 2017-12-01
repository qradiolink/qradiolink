// Written by Adrian Musceac YO8RZZ , started March 2016.
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


#ifndef GR_MODEM_H
#define GR_MODEM_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QtEndian>
#include <QMutex>
#include <QCoreApplication>
#include <string>
#include "ext/utils.h"
#include "sslclient.h"
#include "config_defines.h"
#include "settings.h"
#include "station.h"
#include "modem_types.h"
#include "gr/gr_mod_base.h"
#include "gr/gr_demod_base.h"
#include "gr_mod_gmsk.h"
#include "gr_demod_gmsk.h"
#include "gr_mod_bpsk.h"
#include "gr_demod_bpsk.h"
#include <gnuradio/qtgui/number_sink.h>
#include <gnuradio/qtgui/const_sink_c.h>
#include <gnuradio/qtgui/sink_c.h>


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// posix interrupt timers
#include <time.h>

// needed for usleep
#include <unistd.h>

// for memset
#include <strings.h>
#include <math.h>

// ioctl for CS driving serial port (PTT)
#include <sys/ioctl.h>

// for serial port out (PTT)
#include <termios.h>

#include <errno.h>


class gr_modem : public QObject
{
    Q_OBJECT
public:
    enum
    {
        FrameTypeNone,
        FrameTypeVoice,
        FrameTypeText,
        FrameTypeData,
        FrameTypeVideo,
        FrameTypeStart,
        FrameTypeCallsign,
        FrameTypeEnd
    };
    explicit gr_modem(Settings *settings, gr::qtgui::sink_c::sptr fft_gui, gr::qtgui::const_sink_c::sptr const_gui,
                      gr::qtgui::number_sink::sptr rssi_gui, QObject *parent = 0);
    ~gr_modem();
    long _frequency_found;
    long _requested_frequency_hz;
    void demodulateAnalog();

    void sendCallsign(int size, QString callsign);
signals:
    void pcmAudio(std::vector<float>* pcm);
    void digitalAudio(unsigned char *c2data, int size);
    void videoData(unsigned char *video_data, int size);
    void netData(unsigned char *net_data, int size);
    void demodulated_audio(short *pcm, short size);
    void textReceived(QString text);
    void callsignReceived(QString text);
    void audioFrameReceived();
    void dataFrameReceived();
    void syncIssues();
    void receiveEnd();
    void endAudioTransmission();
public slots:
    void processPCMAudio(float *audio_data, int size);
    void processAudioData(unsigned char *data, int size);
    void processVideoData(unsigned char *data, int size);
    void processNetData(unsigned char *data, int size);
    void demodulate();
    void startTransmission(QString callsign, int size);
    void endTransmission(QString callsign, int size);
    void textData(QString text);
    void initTX(int modem_type, std::string device_args, std::string device_antenna, int freq_corr);
    void initRX(int modem_type, std::string device_args, std::string device_antenna, int freq_corr);
    void deinitTX(int modem_type);
    void deinitRX(int modem_type);
    void toggleRxMode(int modem_type);
    void toggleTxMode(int modem_type);
    void tune(long center_freq);
    void tuneTx(long center_freq);
    void startRX();
    void stopRX();
    void startTX();
    void stopTX();
    void setTxPower(float value);
    void setSquelch(int value);
    void setRxSensitivity(float value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void enableGUIConst(bool value);
    void enableGUIFFT(bool value);
    double getFreqGUI();
private:

    Settings *_settings;
    quint64 _sequence_number;
    bool _transmitting;
    std::vector<unsigned char>* frame(unsigned char *encoded_audio, int data_size, int frame_type=FrameTypeVoice);
    void processReceivedData(unsigned char* received_data, int current_frame_type);
    void handleStreamEnd();
    int findSync(unsigned char bit);
    void transmit(QVector<std::vector<unsigned char>*> frames);
    void synchronize(int v_size, std::vector<unsigned char> *data);

    gr_mod_base *_gr_mod_base;
    gr_demod_base *_gr_demod_base;
    gr_mod_gmsk *_gr_mod_gmsk;
    gr_demod_gmsk *_gr_demod_gmsk;
    gr_mod_bpsk *_gr_mod_bpsk;
    gr_demod_bpsk *_gr_demod_bpsk;

    int _modem_type_rx;
    int _modem_type_tx;
    int _frame_length;
    quint64 _frame_counter;
    quint8 _last_frame_type;
    bool _sync_found;
    int _current_frame_type;
    long _bit_buf_index;
    unsigned char *_bit_buf;
    int _bit_buf_len;
    unsigned long long _shift_reg;

    gr::qtgui::const_sink_c::sptr _const_gui;
    gr::qtgui::number_sink::sptr _rssi_gui;
    gr::qtgui::sink_c::sptr _fft_gui;


};

#endif // GR_MODEM_H
