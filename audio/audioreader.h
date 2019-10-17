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

#ifndef AUDIOREADER_H
#define AUDIOREADER_H

#include <QCoreApplication>
#include <QMutex>
#include <QAudioInput>
#include <QDebug>
#include "audio/audioprocessor.h"
#include "settings.h"
#include "logger.h"

class AudioReader : public QObject
{
    Q_OBJECT
public:
    explicit AudioReader(const Settings *settings, Logger *logger, QObject *parent = 0);

signals:
    void finished();
    void audioPCM(short *pcm, int bytes, int vad, bool radio_only);

public slots:
    void run();
    void setReadMode(bool capture, bool preprocess, int audio_mode);
    void stop();

private:
    const Settings *_settings;
    Logger *_logger;
    QByteArray *_buffer;
    bool _working;
    bool _capture_audio;
    bool _read_preprocess;
    int _read_audio_mode;
    QMutex _mutex;

};

#endif // AUDIOREADER_H
