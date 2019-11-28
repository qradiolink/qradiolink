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

#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QMutex>
#include <sndfile.h>
#include "settings.h"
#include "logger.h"

class AudioRecorder : public QObject
{
    Q_OBJECT
public:
    explicit AudioRecorder(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~AudioRecorder();
signals:

public slots:
    void startRecording();
    void stopRecording();
    void writeSamples(short *samples, int bufsize);

private:
    const Settings *_settings;
    Logger *_logger;
    bool _recording;
    SNDFILE *_snd_out_file;
    SF_INFO _sfinfo ;
    QMutex _mutex;
};

#endif // AUDIORECORDER_H
