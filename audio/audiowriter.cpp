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
    _recorder = new AudioRecorder(settings, logger);
    _working = true;
    _restart = false;
    _record_audio = false;
}

AudioWriter::~AudioWriter()
{

    _rx_sample_queue->clear();
    delete _rx_sample_queue;
    delete _recorder;
}

void AudioWriter::stop()
{
    _working = false;
}

void AudioWriter::restart()
{
    _working = false;
    _restart = true;
}

void AudioWriter::recordAudio(bool value)
{
    _mutex.lock();
    _record_audio = value;
    _mutex.unlock();
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
    start:
    _working = true;
    bool recording = false;
    int frame_counter = 0;
    // FIXME: support different frame durations
    AudioProcessor *processor = new AudioProcessor(_settings);
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
    if (!device.isFormatSupported(format))
    {
        delete processor;
        _logger->log(Logger::LogLevelCritical, "Raw audio format not supported by backend, cannot play audio.");
        struct timespec time_to_sleep = {1, 40000000L };
        nanosleep(&time_to_sleep, NULL);
        goto start;
    }
    _logger->log(Logger::LogLevelInfo, QString("Using audio output device %1").arg(device.deviceName()));
    QAudioOutput *audio_writer = new QAudioOutput(device, format, this);
    // FIXME: bad hack, not needed
    /*
    QObject::connect(audio_writer, SIGNAL(stateChanged(QAudio::State)),
                     this, SLOT(processStateChange(QAudio::State)), Qt::DirectConnection);
    */
    audio_writer->setBufferSize(4096);
    QIODevice *audio_dev = audio_writer->start();

    while(_working)
    {
        QCoreApplication::processEvents();
        _mutex.lock();
        int size = _rx_sample_queue->size();
        if(_record_audio && !recording)
        {
            _recorder->startRecording();
            recording = true;
        }
        if(recording && !_record_audio)
        {
            _recorder->stopRecording();
            recording = false;
        }
        _mutex.unlock();
        if(size > 0)
        {
            frame_counter = 50;
            for(int i=0;i< size;i++)
            {
                audio_samples *samp = _rx_sample_queue->at(i);
                int bytes = samp->bytes;
                bool preprocess = samp->preprocess;
                int audio_mode = samp->audio_mode;
                if(bytes <= 640)
                {
                    short *pcm = new short[bytes/sizeof(short)];
                    memcpy(pcm, samp->pcm, bytes);
                    processor->write_preprocess(pcm, bytes, preprocess, audio_mode);
                    audio_dev->write((char*)pcm, bytes);
                    if(recording)
                    {
                        _recorder->writeSamples(pcm, bytes/sizeof(short));
                    }
                    delete[] pcm;
                    /// time it takes for the packet to be played without overflow
                    long play_time = 1000/(8000/(bytes/sizeof(short))) * 1000000L;
                    if(play_time > 2000000L)
                        play_time -= 2000000L;
                    struct timespec time_to_sleep = {0, play_time };
                    nanosleep(&time_to_sleep, NULL);
                }
                else
                {
                    int frames = bytes / 640;
                    int leftover = bytes % 640;
                    for(int i=0;i<frames;i++)
                    {
                        short *pcm = new short[320];
                        memcpy(pcm, samp->pcm+(320*i), 640);
                        processor->write_preprocess(pcm, 640, preprocess, audio_mode);
                        audio_dev->write((char*)pcm, 640);
                        if(recording)
                        {
                            _recorder->writeSamples(pcm, 640/sizeof(short));
                        }
                        delete[] pcm;
                        /// time it takes for the packet to be played without overflow
                        struct timespec time_to_sleep = {0, 39000000L };
                        nanosleep(&time_to_sleep, NULL);
                    }
                    if(leftover > 1)
                    {
                        short *pcm = new short[leftover/sizeof(short)];
                        memcpy(pcm, samp->pcm+(320*frames), leftover);
                        processor->write_preprocess(pcm, leftover, preprocess, audio_mode);
                        audio_dev->write((char*)pcm, leftover);
                        if(recording)
                        {
                            _recorder->writeSamples(pcm, leftover/sizeof(short));
                        }
                        delete[] pcm;
                        /// time it takes for the packet to be played without overflow
                        long play_time = 1000/(8000/(leftover/sizeof(short))) * 1000000L;
                        struct timespec time_to_sleep = {0, play_time };
                        nanosleep(&time_to_sleep, NULL);
                    }
                }
                delete[] samp->pcm;
                delete samp;
            }
            _mutex.lock();
            _rx_sample_queue->remove(0, size);
            _mutex.unlock();

        }
        else
        {
            if(frame_counter > 0)
            {
                /// recent audio packet
                frame_counter--;
                struct timespec time_to_sleep = {0, 1000000L };
                nanosleep(&time_to_sleep, NULL);
            }
            else if(frame_counter == 0)
            {
                /// Idle time
                struct timespec time_to_sleep = {0, 40000000L };
                nanosleep(&time_to_sleep, NULL);
            }
        }
    }
    _rx_sample_queue->clear();
    audio_dev->close();
    audio_writer->stop();
    delete audio_writer;
    delete processor;
    if(_restart)
    {
        _restart = false;
        goto start;
    }
    emit finished();
}

void AudioWriter::processStateChange(QAudio::State state)
{
    /// Not needed...
    if(state == QAudio::IdleState)
    {
        QAudioOutput *audio_writer = reinterpret_cast<QAudioOutput*>(this->sender());
        audio_writer->resume();
    }
}
