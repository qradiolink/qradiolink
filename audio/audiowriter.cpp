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


#include "audiowriter.h"

AudioWriter::AudioWriter(const Settings *settings, Logger *logger, QObject *parent) :
    QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _rx_sample_queue = new QVector<audio_samples*>;
    _working = true;
}

AudioWriter::~AudioWriter()
{

    _rx_sample_queue->clear();
    delete _rx_sample_queue;
}

void AudioWriter::stop()
{
    _working = false;
}


void AudioWriter::writePCM(short *pcm, int bytes, bool preprocess, int audio_mode)
{

    audio_samples *samp = new audio_samples;
    samp->pcm = pcm;
    samp->bytes = bytes;
    samp->preprocess = preprocess;
    samp->audio_mode = audio_mode;
    _mutex.lock();
    _rx_sample_queue->push_back(samp);
    _mutex.unlock();
}

void AudioWriter::run()
{
    // FIXME: support different frame durations
    AudioProcessor *processor = new AudioProcessor;
    QAudioFormat format;
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo device = QAudioDeviceInfo::defaultOutputDevice();
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for(int i = 0;i<devices.size();i++)
    {
        if(_settings->audio_output_device == devices.at(i).deviceName())
        {
            device = devices.at(i);
            break;
        }
    }
    if (!device.isFormatSupported(format)) {
       _logger->log(Logger::LogLevelCritical, "Raw audio format not supported by backend, cannot play audio.");
       return;
    }

    QAudioOutput *audio_writer = new QAudioOutput(device, format, this);
    audio_writer->setBufferSize(4096);
    QIODevice *audio_dev = audio_writer->start();

    while(_working)
    {
        QCoreApplication::processEvents();
        _mutex.lock();
        int size = _rx_sample_queue->size();
        _mutex.unlock();
        if(size > 0)
        {
            for(int i=0;i< size;i++)
            {
                audio_samples *samp = _rx_sample_queue->at(i);
                int bytes = samp->bytes;
                bool preprocess = samp->preprocess;
                int audio_mode = samp->audio_mode;
                short *pcm = new short[bytes/sizeof(short)];
                memcpy(pcm, samp->pcm, samp->bytes);
                delete[] samp->pcm;
                delete samp;
                processor->write_preprocess(pcm, bytes, preprocess, audio_mode);
                audio_dev->write((char*)pcm, bytes);
            }
            _mutex.lock();
            _rx_sample_queue->remove(0, size);
            _mutex.unlock();

        }
        else
        {
            struct timespec time_to_sleep = {0, 35000000L };
            nanosleep(&time_to_sleep, NULL);
        }
    }
    audio_writer->stop();
    delete audio_writer;
    delete processor;
}
