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

#ifndef DTMFDECODER_H
#define DTMFDECODER_H


#include <QObject>
#include <QtDebug>
#include <QCoreApplication>
#include <QDateTime>
#include "audio/audiointerface.h"
#include "config_defines.h"
#include "ext/Goertzel.h"
#include "settings.h"

class DtmfDecoder : public QObject
{
    Q_OBJECT
public:
    explicit DtmfDecoder(Settings *settings, QObject *parent = 0);
    ~DtmfDecoder();
    void stop();
    void process(bool p);
    
signals:
    void finished();
    void haveCall(QVector<char>*call);
    void haveCommand(QVector<char>*command);
    
public slots:
    void run();
    void resetInput();

private:
    bool _stop;


    /**
     * @brief decoding DTMF audio tones
     * @param buf
     * @param BUFSIZE
     * @param SAMP_RATE
     * @param treshhold_audio_power
     * @return
     */
    char decode(float *buf, int buffer_size, int samp_rate, float treshhold_audio_power, float tone_difference);
    char newDecode(float *buf,int buffer_size,int samp_rate, float treshhold_audio_power, float tone_difference);
    /**
     * @brief Statistical analysis of char buffer
     */
    void analyse(int analysis_buffer);
    int _dtmf_frequencies[10];
    QVector<char> *_dtmf_sequence;
    QVector<char> *_dtmf_command;
    char _current_letter;
    char _previous_letter;
    bool _processing;
    bool _receiving;
    Settings *_settings;


};



#endif // DTMFDECODER_H
