// Written by Adrian Musceac YO8RZZ at gmail dot com, started October 2013.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
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

#ifndef GMSKMODEM_H
#define GMSKMODEM_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QtEndian>
#include <QCoreApplication>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include "ext/utils.h"
#include "sslclient.h"
#include "audiointerface.h"
#include "audioencoder.h"
#include "opus/opus.h"
#include "config_defines.h"
#include "settings.h"
#include "station.h"
#include "alsaaudio.h"


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

// c2gmsk API
extern "C"
{
#include <c2gmsk.h>
}

struct c2gmsk_msg {
    int tod; // type of data
    int datasize; // whatever size it is
    unsigned char data[]; // unknown size, will not be included in sizeof
};
class GMSKModem : public QObject
{
    Q_OBJECT
public:
    explicit GMSKModem(Settings *settings, AlsaAudio *audio, AudioInterface *audio2, QObject *parent = 0);
    ~GMSKModem();

signals:
    void pcmAudio(short *pcm, short size);
    void demodulated_audio(short *pcm, short size);
    void textReceived(QString text);
    void audioFrameReceived();
    void dataFrameReceived();
    void syncIssues();
    void receiveEnd();
public slots:
    void processAudio(short *audiobuffer, short audiobuffersize);
    void demodulate(short *pcm, short size);
    void startTransmission();
    void endTransmission();
    void textData(QString text);
private:
    struct c2gmsk_session *_gmsk_session = NULL;
    struct c2gmsk_param _parameters;
    struct c2gmsk_msgchain * _chain=NULL,  ** _pchain; // pointer to pointer of message chain

    OpusEncoder *_opus_encoder;
    OpusDecoder *_opus_decoder;
    AudioEncoder *_codec;
    Settings *_settings;
    quint64 _sequence_number;
    bool _transmitting;
    void modulate(unsigned char *encoded_audio, int data_size);
    void processReceivedData(unsigned char* all_data);
    void interleaveHeader();
    void handleStreamEnd(unsigned char *all_data);
    AlsaAudio *_audio;
    AudioInterface *_audio2;
    quint64 _frame_counter;
    quint8 _last_frame_type;
    enum
    {
        FrameTypeNone,
        FrameTypeVoice,
        FrameTypeText,
        FrameTypeData
    };


};

#endif // GMSKMODEM_H
