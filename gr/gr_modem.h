// Written by Adrian Musceac YO8RZZ at gmail dot com, started March 2016.
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
#include "gr_mod_gmsk.h"
#include "gr_demod_gmsk.h"
#include "gr_mod_bpsk.h"
#include "gr_demod_bpsk.h"
#include "gr_mod_bpsk_sdr.h"
#include "gr_demod_bpsk_sdr.h"
#include "gr_mod_qpsk_sdr.h"
#include "gr_demod_qpsk_sdr.h"
#include <gnuradio/qtgui/number_sink.h>


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

namespace gr_modem_types {
    enum
    {
        ModemTypeBPSK2000,
        ModemTypeQPSK20000,
        ModemTypeAnalog2000
    };
}

class gr_modem : public QObject
{
    Q_OBJECT
public:
    explicit gr_modem(Settings *settings, gr::qtgui::const_sink_c::sptr const_gui,
                      gr::qtgui::number_sink::sptr rssi_gui, QObject *parent = 0);
    ~gr_modem();
    bool _frequency_found;
    long _requested_frequency_hz;
signals:
    void pcmAudio(short *pcm, short size);
    void codec2Audio(unsigned char *c2data, short size);
    void demodulated_audio(short *pcm, short size);
    void textReceived(QString text);
    void audioFrameReceived();
    void dataFrameReceived();
    void syncIssues();
    void receiveEnd();
public slots:
    void processC2Data(unsigned char *data, short size);
    void demodulate();
    void startTransmission();
    void endTransmission();
    void textData(QString text);
    void initTX(int modem_type);
    void initRX(int modem_type);
    void deinitTX(int modem_type);
    void deinitRX(int modem_type);
    void tune(long center_freq, bool sync=false);
    void startRX();
    void stopRX();
    void startTX();
    void stopTX();
    void setTxPower(int value);
private:

    Settings *_settings;
    quint64 _sequence_number;
    bool _transmitting;
    std::vector<unsigned char>* frame(unsigned char *encoded_audio, int data_size, int frame_type=FrameTypeVoice);
    void processReceivedData(unsigned char* received_data, int current_frame_type);
    void handleStreamEnd();
    int findSync(unsigned char bit);
    void transmit(QVector<std::vector<unsigned char>*> frames);


    gr_mod_gmsk *_gr_mod_gmsk;
    gr_demod_gmsk *_gr_demod_gmsk;
    gr_mod_bpsk *_gr_mod_bpsk;
    gr_mod_bpsk_sdr *_gr_mod_bpsk_sdr;
    gr_mod_qpsk_sdr *_gr_mod_qpsk_sdr;
    gr_demod_bpsk *_gr_demod_bpsk;
    gr_demod_bpsk_sdr *_gr_demod_bpsk_sdr;
    gr_demod_qpsk_sdr *_gr_demod_qpsk_sdr;
    int _modem_type;
    int _frame_length;
    quint64 _frame_counter;
    quint8 _last_frame_type;
    bool _sync_found;
    int _current_frame_type;
    long _bit_buf_index;
    unsigned char *_bit_buf;
    int _bit_buf_len;
    unsigned long long _shift_reg;
    bool _stream_started;
    bool _stream_ended;

    gr::qtgui::const_sink_c::sptr _const_gui;
    gr::qtgui::number_sink::sptr _rssi_gui;

    enum
    {
        FrameTypeNone,
        FrameTypeVoice,
        FrameTypeText,
        FrameTypeData,
        FrameTypeStart,
        FrameTypeEnd
    };



};

#endif // GR_MODEM_H
