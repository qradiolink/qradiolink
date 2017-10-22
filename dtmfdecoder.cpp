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

#include "dtmfdecoder.h"





DtmfDecoder::DtmfDecoder(Settings *settings, QObject *parent) :
    QObject(parent)
{
    _stop=false;
    _dtmf_frequencies[0]= 697;
    _dtmf_frequencies[1]= 770;
    _dtmf_frequencies[2]= 852;
    _dtmf_frequencies[3]= 941;
    _dtmf_frequencies[4]= 1100;
    _dtmf_frequencies[5]= 1209;
    _dtmf_frequencies[6]= 1336;
    _dtmf_frequencies[7]= 1477;
    _dtmf_frequencies[8]= 1633;
    _dtmf_frequencies[9]= 2500;
    _dtmf_sequence = new QVector<char>;
    _dtmf_command = new QVector<char>;
    _current_letter = ' ';
    _previous_letter= ' ';
    _processing = true;
    _receiving = false;
    _settings = settings;

}

DtmfDecoder::~DtmfDecoder()
{
    _dtmf_command->clear();
    _dtmf_sequence->clear();
    delete _dtmf_command;
    delete _dtmf_sequence;
}


void DtmfDecoder::stop()
{
    _stop = true;
}

void DtmfDecoder::process(bool p)
{
    _processing = p;
}

void DtmfDecoder::resetInput()
{
    process(true);
}

void DtmfDecoder::run()
{
    float cw_tone_freq = 900.0;
    Q_UNUSED(cw_tone_freq);
    int buffer_size = 512;
    int samp_rate = 8000;
    float treshhold_audio_power = 100.0;
    float tone_difference = 6.0; //dB
    int analysis_buffer = 25;
    char call_key='C';
    char call_direct_key='Q';
    char command_key='D';
    char clear_key = '*';

    AudioInterface *audio= new AudioInterface(0,samp_rate,1,0);

    while(true)
    {
        int last_time = 0;
        int time = QDateTime::currentDateTime().toTime_t();
        usleep(10000);
        QCoreApplication::processEvents();


        float buf[buffer_size];
        memset(buf,0,buffer_size*sizeof(float));

        audio->read(buf, buffer_size);

        char letter = '?';

        letter = newDecode(buf,buffer_size,samp_rate, treshhold_audio_power, tone_difference);

        /** this function uses code with unknown license
        letter = decode(buf,buffer_size,samp_rate, treshhold_audio_power, tone_difference);
        */

        if(_dtmf_sequence->size()>analysis_buffer)
        {
            _dtmf_sequence->remove(0);
        }
        _dtmf_sequence->append(letter);
        // make a statistical analysis of the buffer
        analyse(analysis_buffer);


        if(_current_letter==' ')
        {
            _previous_letter = _current_letter;
            continue;
        }
        if(_previous_letter==_current_letter) continue;

        qDebug() << QString(_current_letter);

        if(_current_letter==clear_key)
        {
            _dtmf_command->clear();

        }
        else if((_current_letter==call_key) || _current_letter==call_direct_key)
        {

            _dtmf_command->append(_current_letter);

            QVector<char> *dtmf = new QVector<char>;
            for(int i=0;i<_dtmf_command->size();i++)
            {
                dtmf->append(_dtmf_command->at(i));
            }
            emit haveCall(dtmf);
            _dtmf_command->clear();
        }
        else if(_current_letter==command_key)
        {
            _dtmf_command->append(_current_letter);
            QVector<char> *dtmf = new QVector<char>;
            for(int i=0;i<_dtmf_command->size();i++)
            {
                dtmf->append(_dtmf_command->at(i));
            }

            emit haveCommand(dtmf);
            _dtmf_command->clear();
        }
        else
        {
            last_time = QDateTime::currentDateTime().toTime_t();
            _dtmf_command->append(_current_letter);
        }

        if(((time - last_time) > 30) && (_dtmf_command->size() > 0))
        {
            //_dtmf_command->clear();
            last_time = time;
        }
        _previous_letter=_current_letter;

    }

    finish:
    delete audio;
    emit finished();
}

char DtmfDecoder::newDecode(float *buf,int buffer_size,int samp_rate, float treshhold_audio_power, float tone_difference)
{
    Q_UNUSED(tone_difference);
    int tones[2];
    tones[0] = 0;
    tones[1] = 0;
    float largest_tone_power = 0.0;
    for(int i=0;i<5;i++)
    {
        Goertzel gk(static_cast<float>(_dtmf_frequencies[i]), samp_rate);
        for(int j=0;j<buffer_size;j++)
        {
            buf[j] = (buf[j] > 1.0) ? 0.0 : buf[j];
            buf[j] = (buf[j] < -1.0) ? 0.0 : buf[j];
            gk.calc(buf[j]);
        }
        float tone_power = gk.magnitudeSquared();
        if(!std::isfinite(tone_power))
        {
            qDebug() << "infinite value";
        }

        if(tone_power != tone_power) continue; // no ffast-math here
        if(tone_power < largest_tone_power) continue;
        if(tone_power < treshhold_audio_power) continue;

        tones[0] = _dtmf_frequencies[i];
        largest_tone_power = tone_power;
    }
    largest_tone_power = 0.0;
    for(int i=5;i<9;i++)
    {
        Goertzel gk(static_cast<float>(_dtmf_frequencies[i]), samp_rate);
        for(int j=0;j<buffer_size;j++)
        {
            buf[j] = (buf[j] > 1.0) ? 0.0 : buf[j];
            buf[j] = (buf[j] < -1.0) ? 0.0 : buf[j];
            gk.calc(buf[j]);
        }
        float tone_power = gk.magnitudeSquared();
        if(!std::isfinite(tone_power))
        {
            qDebug() << "infinite value";
        }

        if(tone_power != tone_power) continue; // no ffast-math here
        if(tone_power < largest_tone_power) continue;
        if(tone_power < treshhold_audio_power) continue;

        tones[1] = _dtmf_frequencies[i];
        largest_tone_power = tone_power;
    }
    char letter;
    switch(tones[0])
    {
    case 697:
        switch(tones[1])
        {
        case 1209:
            letter = '1';
            break;
        case 1336:
            letter = '2';
            break;
        case 1477:
            letter = '3';
            break;
        case 1633:
            letter = 'A';
            break;
        default:
            letter = ' ';
        }

        break;
    case 770:
        switch(tones[1])
        {
        case 1209:
            letter = '4';
            break;
        case 1336:
            letter = '5';
            break;
        case 1477:
            letter = '6';
            break;
        case 1633:
            letter = 'B';
            break;
        default:
            letter = ' ';
        }
        break;
    case 852:
        switch(tones[1])
        {
        case 1209:
            letter = '7';
            break;
        case 1336:
            letter = '8';
            break;
        case 1477:
            letter = '9';
            break;
        case 1633:
            letter = 'C';
            break;
        default:
            letter = ' ';
        }
        break;
    case 941:
        switch(tones[1])
        {
        case 1209:
            letter = '*';
            break;
        case 1336:
            letter = '0';
            break;
        case 1477:
            letter = '#';
            break;
        case 1633:
            letter = 'D';
            break;
        default:
            letter = ' ';
        }
        break;
    default:
        letter = ' ';
    }
    return letter;
}

void DtmfDecoder::analyse(int analysis_buffer)
{

    if(_dtmf_sequence->size()<analysis_buffer)
        return;


    int x1,x2,x3,x4,x5,x6,x7,x8,x9,x0,xa,xb,xc,xd,xs,xq,xx;
    x1=x2=x3=x4=x5=x6=x7=x8=x9=x0=xa=xb=xc=xd=xs=xq=xx=0;
    for(int o=0;o<_dtmf_sequence->size();++o)
    {
        char letter = _dtmf_sequence->at(o);
        if(letter == ' ')
            xx++;
        if(letter == '1')
            x1++;
        if(letter == '2')
            x2++;
        if(letter == '3')
            x3++;
        if(letter == '4')
            x4++;
        if(letter == '5')
            x5++;
        if(letter == '6')
            x6++;
        if(letter == '7')
            x7++;
        if(letter == '8')
            x8++;
        if(letter == '9')
            x9++;
        if(letter == '0')
            x0++;
        if(letter == 'A')
            xa++;
        if(letter == 'B')
            xb++;
        if(letter == 'C')
            xc++;
        if(letter == 'D')
            xd++;
        if(letter == '*')
            xs++;
        if(letter == '#')
            xq++;
    }


    if(xx > round(_dtmf_sequence->size()/2))
    {
        /*
        for(int i =analysis_buffer-1;i>analysis_buffer-5;i--)
        {
            if(_dtmf_sequence->at(i)!=' ')
                //wait for another iteration
                _current_letter = ' ';
                return;
        }
        */

        _current_letter = ' ';
        return;
    }


    else
    {
        if(x1 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '1';
            return;
        }
        if(x2 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '2';
            return;
        }
        if(x3 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '3';
            return;
        }
        if(x4 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '4';
            return;
        }
        if(x5 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '5';
            return;
        }
        if(x6 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '6';
            return;
        }
        if(x7 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '7';
            return;
        }
        if(x8 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '8';
            return;
        }
        if(x9 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '9';
            return;
        }
        if(x0 > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '0';
            return;
        }
        if(xa > round(_dtmf_sequence->size()/2))
        {
            _current_letter = 'A';
            return;
        }
        if(xb > round(_dtmf_sequence->size()/2))
        {
            _current_letter = 'B';
            return;
        }
        if(xc > round(_dtmf_sequence->size()/2))
        {
            _current_letter = 'C';
            return;
        }
        if(xd > round(_dtmf_sequence->size()/2))
        {
            _current_letter = 'D';
            return;
        }
        if(xs > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '*';
            return;
        }
        if(xq > round(_dtmf_sequence->size()/2))
        {
            _current_letter = '#';
            return;
        }
        // no letter has prevalence, wait for another iteration

        return;
    }
}



