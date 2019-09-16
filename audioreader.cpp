#include "audioreader.h"

AudioReader::AudioReader(QObject *parent) :
    QObject(parent)
{

    _working = true;
    _capture_audio = false;
    _read_audio_mode = AudioInterface::AUDIO_MODE_ANALOG;
    _read_preprocess = false;
}


void AudioReader::stop()
{
    _working = false;
}

void AudioReader::setReadMode(bool capture, bool preprocess, int audio_mode)
{
    _mutex.lock();
    _capture_audio = capture;
    _read_audio_mode = audio_mode;
    _read_preprocess = preprocess;
    _mutex.unlock();
}



void AudioReader::run()
{
    _audio_reader = new AudioInterface;
    while(_working)
    {
        QCoreApplication::processEvents();

        _mutex.lock();
        bool capture = _capture_audio;
        bool preprocess = _read_preprocess;
        int audio_mode = _read_audio_mode;
        _mutex.unlock();

        if(capture)
        {
            // FIXME: support different frame durations
            int audiobuffer_size = 640; //40 ms @ 8k
            short *audiobuffer = new short[audiobuffer_size/sizeof(short)];
            int vad = _audio_reader->read_short(audiobuffer,audiobuffer_size, preprocess, audio_mode);
            emit audioPCM(audiobuffer, audiobuffer_size, vad, false);

            struct timespec time_to_sleep = {0, 2000L };
            nanosleep(&time_to_sleep, NULL);
        }
        else
        {
            struct timespec time_to_sleep = {0, 5000000L };
            nanosleep(&time_to_sleep, NULL);
        }



    }
    delete _audio_reader;
}
