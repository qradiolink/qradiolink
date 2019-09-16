#include "audiowriter.h"

AudioWriter::AudioWriter(QObject *parent) :
    QObject(parent)
{

    _rx_sample_queue = new std::vector<audio_samples*>;
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
    _audio_writer = new AudioInterface;
    while(_working)
    {
        QCoreApplication::processEvents();
        int size = _rx_sample_queue->size();
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
            // FIXME: compressor segfaults across threads
            _audio_writer->write_short(pcm, bytes, false, audio_mode);

        }
        _mutex.lock();
        _rx_sample_queue->clear();
        _mutex.unlock();

        struct timespec time_to_sleep = {0, 1000000L };
        nanosleep(&time_to_sleep, NULL);
    }
    delete _audio_writer;
}
