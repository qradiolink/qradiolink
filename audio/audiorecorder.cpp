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

#include "audiorecorder.h"

AudioRecorder::AudioRecorder(const Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _recording = false;
    _sfinfo.format   = SF_FORMAT_FLAC | SF_FORMAT_PCM_16 ;
    _sfinfo.samplerate = 8000;
    _sfinfo.channels = 1 ;
}

AudioRecorder::~AudioRecorder()
{
    stopRecording();
}

void AudioRecorder::startRecording()
{
    QString time= QDateTime::currentDateTime().toString(
                "d_MMM_yyyy_hh-mm-ss");
    QString filename = QString("%1/%2.%3").arg(_settings->audio_record_path).arg(time).arg("flac");
    if (! (_snd_out_file = sf_open(filename.toStdString().c_str(), SFM_WRITE, &_sfinfo)))
    {
        _logger->log(Logger::LogLevelCritical, QString("Could not open audio file %1 for recording").arg(
                         filename));
    }
    _logger->log(Logger::LogLevelInfo, QString("Starting audio recording to file %1").arg(
                     filename));
    _recording = true;
}

void AudioRecorder::writeSamples(short *samples, int bufsize)
{
    if(!_recording)
        return;
    sf_write_short(_snd_out_file, samples, (sf_count_t)bufsize);

}

void AudioRecorder::stopRecording()
{
    if(!_recording)
        return;
    _logger->log(Logger::LogLevelInfo, QString("Stopping audio recording"));
    sf_write_sync(_snd_out_file) ;
    sf_close(_snd_out_file);
    _recording = false;
}


