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

#ifndef AUDIOWRITER_H
#define AUDIOWRITER_H

#include <QObject>
#include <QVector>
#include <QCoreApplication>
#include <QMutex>
#include <QAudioOutput>
#include "audio/audioprocessor.h"
#include "settings.h"
#include "logger.h"

class AudioWriter : public QObject
{
    Q_OBJECT
public:
    explicit AudioWriter(const Settings *settings, Logger *logger, QObject *parent = 0);
    ~AudioWriter();

signals:
    void finished();

public slots:
    void run();
    void writePCM(short *pcm, int bytes, bool preprocess, int audio_mode);
    void stop();

private:
    struct audio_samples
    {
        audio_samples() : pcm(0), bytes(0), preprocess(false), audio_mode(0) {}
        short *pcm;
        int bytes;
        bool preprocess;
        int audio_mode;
    };
    const Settings *_settings;
    Logger *_logger;
    QVector<audio_samples*> *_rx_sample_queue;
    bool _working;
    QMutex _mutex;

};

#endif // AUDIOWRITER_H
