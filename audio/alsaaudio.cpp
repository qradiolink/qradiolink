// Written by Adrian Musceac YO8RZZ , started October 2013.
// Code based on work by Kamal Mostafa <kamal@whence.com>
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

#include "alsaaudio.h"

AlsaAudio::AlsaAudio(QString device, int sample_rate)
{
    _pcm_rec = NULL;
    _pcm_play = NULL;
    if(!initDevices(device))
    {
        configureDevices(sample_rate);
    }

}

AlsaAudio::~AlsaAudio()
{
    snd_pcm_drain(_pcm_play);
    snd_pcm_close(_pcm_play);
    snd_pcm_drain(_pcm_rec);
    snd_pcm_close(_pcm_rec);
}


int AlsaAudio::initDevices(QString device)
{
    QString device_rec;
    QString device_play;
    if(device != "default")
    {
        device_rec = device + "_rec";
        device_play = device + "_play";
    }
    else
    {
        device_rec = device;
        device_play = device;
    }
    int error = snd_pcm_open(&_pcm_rec,
            device_rec.toStdString().c_str(),
            SND_PCM_STREAM_CAPTURE,
            0 /*mode*/);
    if (error)
    {
        fprintf(stderr, "E: Cannot create ALSA stream: %s\n", snd_strerror(error));
        return 1;
    }
    error = snd_pcm_open(&_pcm_play,
            device_play.toStdString().c_str(),
            SND_PCM_STREAM_PLAYBACK,
            0 /*mode*/);
    if (error)
    {
        fprintf(stderr, "E: Cannot create ALSA stream: %s\n", snd_strerror(error));
        return 1;
    }
    return 0;
}

int AlsaAudio::configureDevices(int sample_rate)
{
    snd_pcm_format_t pcm_format = SND_PCM_FORMAT_S16;
    int error = snd_pcm_set_params(_pcm_rec,
            pcm_format,
            SND_PCM_ACCESS_RW_INTERLEAVED,
            1,
            sample_rate,
            1 /* soft_resample (allow) */,
            100000 /* latency (us) */);
    if (error)
    {
        fprintf(stderr, "E: %s\n", snd_strerror(error));
        snd_pcm_close(_pcm_rec);
        _pcm_rec = NULL;
    }
    error = snd_pcm_set_params(_pcm_play,
            pcm_format,
            SND_PCM_ACCESS_RW_INTERLEAVED,
            1,
            sample_rate,
            1 /* soft_resample (allow) */,
            100000 /* latency (us) */);
    if (error)
    {
        fprintf(stderr, "E: %s\n", snd_strerror(error));
        snd_pcm_close(_pcm_play);
        _pcm_play = NULL;
    }
    return 0;
}


int AlsaAudio::write_short(short *buf, short bufsize)
{
    if(!_pcm_play)
        return 1;
    ssize_t frames_written = 0;
    snd_pcm_t *pcm = (snd_pcm_t *)_pcm_play;
    while ( frames_written < bufsize ) {
    ssize_t r;
    r = snd_pcm_writei(pcm, buf+frames_written*sizeof(short), bufsize-frames_written);
    if (r < 0) {
        /* recover from e.g. underruns, and try once more */
        snd_pcm_recover(pcm, r, 0 /*silent*/);
        r = snd_pcm_writei(pcm, buf+frames_written*sizeof(short), bufsize-frames_written);
    }
    if (r < 0) {
        fprintf(stderr, "E: %s\n", snd_strerror(frames_written));
        return 1;
    }
    frames_written += r;
    }
    assert (frames_written == bufsize);
    return 0;
}

int AlsaAudio::read_short(short *buf, short bufsize)
{
    if(!_pcm_rec)
        return 1;
    ssize_t frames_read = 0;
    snd_pcm_t *pcm = _pcm_rec;
    while ( frames_read < bufsize ) {
        ssize_t r;
        void * data = buf+frames_read*sizeof(short);
        ssize_t count = bufsize-frames_read;
        r = snd_pcm_readi(pcm, data, count);
        if ( r >= 0 ) {
            frames_read += r;
            if ( r != count )
            fprintf(stderr, "#short+%zd#\n", r);
            continue;
        }
        if (r == -EPIPE) {	// Underrun
            fprintf(stderr, "#");
            snd_pcm_prepare(_pcm_rec);
        } else  {
            //fprintf(stderr, "snd_pcm_readi: %s\n", snd_strerror(r));
            if (r == -EAGAIN || r== -ESTRPIPE)
            snd_pcm_wait(_pcm_rec, 1000);
            else
            return r;
        }
    }
    // fprintf(stderr,("[%zd]\n"), frames_read);
    return 0;
}

