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


#include "audioreader.h"

AudioReader::AudioReader(const Settings *settings, Logger *logger, QObject *parent) :
    QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _working = true;
    _restart = false;
    _capture_audio = false;
    _read_audio_mode = AudioProcessor::AUDIO_MODE_ANALOG;
    _read_preprocess = false;
    _audiobuffer_size = 640; //40 ms @ 8k
}


void AudioReader::stop()
{
    _working = false;
    _logger->log(Logger::LogLevelInfo, QString("Stopping audio input thread"));
}

void AudioReader::restart()
{
    _working = false;
    _restart = true;
}

void AudioReader::setReadMode(bool capture, bool preprocess, int audio_mode, int audiobuffer_size)
{
    _mutex.lock();
    _capture_audio = capture;
    _read_audio_mode = audio_mode;
    _read_preprocess = preprocess;
    _audiobuffer_size = audiobuffer_size;
    _mutex.unlock();
}

void AudioReader::run()
{
    start:
    _working = true;
    AudioProcessor *processor = new AudioProcessor(_settings);
    _buffer = new QByteArray; 
    QAudioFormat format;
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo device = QAudioDeviceInfo::defaultInputDevice();
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i = 0;i<devices.size();i++)
    {
        if(_settings->audio_input_device == devices.at(i).deviceName())
        {
            device = devices.at(i);
            break;
        }
    }
    if (!device.isFormatSupported(format))
    {
        delete processor;
        delete _buffer;
       _logger->log(Logger::LogLevelCritical, "Raw audio format not supported by backend, cannot capture audio.");
       struct timespec time_to_sleep = {1, 40000000L };
       nanosleep(&time_to_sleep, NULL);
       goto start;
    }
    _logger->log(Logger::LogLevelInfo, QString("Using audio input device %1").arg(device.deviceName()));
    QAudioInput *audio_reader = new QAudioInput(device,format, this);
    audio_reader->setBufferSize(4096);
    QIODevice *audio_dev = audio_reader->start();
    while(_working)
    {
        QCoreApplication::processEvents();
        bool capture = _capture_audio;
        bool preprocess = _read_preprocess;
        int audio_mode = _read_audio_mode;
        long sleep_time = 1000000L * (1000L / (8000 / (_audiobuffer_size/2)));
        QByteArray data = audio_dev->readAll();
        if(capture)
        {
            _buffer->append(data);
            if(_buffer->size() >= _audiobuffer_size)
            {
                short *audiobuffer = new short[_audiobuffer_size/sizeof(short)];
                memcpy(audiobuffer, (short*)_buffer->data(), _audiobuffer_size);
                int vad = processor->read_preprocess(audiobuffer, _audiobuffer_size, preprocess, audio_mode);
                emit audioLevel(processor->audio_level);
                emit audioPCM(audiobuffer, _audiobuffer_size, vad, false);
                _buffer->remove(0, _audiobuffer_size);
                struct timespec time_to_sleep = {0, sleep_time - 1000000L };
                nanosleep(&time_to_sleep, NULL);
            }
        }
        else
        {
            struct timespec time_to_sleep = {0, sleep_time };
            nanosleep(&time_to_sleep, NULL);
        }

    }
    audio_dev->close();
    audio_reader->stop();
    _buffer->clear();
    delete audio_reader;
    delete processor;
    delete _buffer;
    if(_restart)
    {
        _restart = false;
        goto start;
    }
    emit finished();
}
