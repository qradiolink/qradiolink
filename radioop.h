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

#ifndef RADIOOP_H
#define RADIOOP_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QDebug>
#include <QCoreApplication>
#if 0
#include <QSoundEffect>
#endif
#include <unistd.h>
#include <math.h>
#include "audio/audiointerface.h"
#include "ext/agc.h"
#include "ext/vox.h"
#include "settings.h"
#include "audio/audioencoder.h"
#if 0
#include "gmskmodem.h"
#endif
#include "audio/alsaaudio.h"
#include "gr/gr_modem.h"


class RadioOp : public QObject
{
    Q_OBJECT
public:
    explicit RadioOp(Settings *settings, QObject *parent = 0);
    ~RadioOp();
signals:
    void finished();
    void printText(QString text);
    void displayReceiveStatus(bool status);
    void displaySyncIssue(bool status);
    void displayTransmitStatus(bool status);
    void displayDataReceiveStatus(bool status);
    void audioData(unsigned char *buf, short size);
public slots:
    void run();
    void startTransmission();
    void endTransmission();
    void textData(QString text, bool repeat = false);
    void stop();
    void textReceived(QString text);
    void audioFrameReceived();
    void dataFrameReceived();
    void receiveEnd();
    void syncIssue();
    void receiveC2Data(unsigned char *data, short size);
    void toggleRX(bool value);
    void toggleTX(bool value);
    void toggleWideband(bool value);
    void tuneFreq(long center_freq);

private:
    bool _stop;
    bool _tx_inited;
    bool _rx_inited;
#if 0
    AlsaAudio *_audio;
#endif
    AudioInterface *_audio;
    Settings *_settings;
    bool _transmitting;
    bool _process_text;
    bool _repeat_text;
    QString _text_out;
    QMutex _mutex;
    QTimer *_led_timer;
    AudioEncoder *_codec;
    gr_modem *_modem;
    bool _wideband;

};

#endif // RADIOOP_H
