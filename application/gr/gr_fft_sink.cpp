// Written by Adrian Musceac YO8RZZ , started March 2016.
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


#include "gr/gr_fft_sink.h"

gr_fft_sink_sptr
make_gr_fft_sink ()
{
    return gnuradio::get_initial_sptr(new gr_fft_sink);
}

gr_fft_sink::gr_fft_sink() :
        gr::sync_block("gr_fft_sink",
                       gr::io_signature::make (1, 1, sizeof (gr_complex)),
                       gr::io_signature::make (0, 0, 0))
{
    _offset = 0;
    _finished = false;
    _data = new std::vector<float>;
    _fft = new gr::fft::fft_complex(_fft_size, true, 4);

}

gr_fft_sink::~gr_fft_sink()
{
    delete _fft;
    delete _data;
}

void gr_fft_sink::flush()
{
    gr::thread::scoped_lock guard(_mutex);
    _data->clear();
}

std::vector<float> *gr_fft_sink::get_data()
{
    gr::thread::scoped_lock guard(_mutex);

    std::vector<float>* data = new std::vector<float>;
    if(_data->size() < 1)
    {
        return data;
    }
    data->reserve(_data->size());
    data->insert(data->end(),_data->begin(),_data->end());
    _data->clear();

    return data;
}

int gr_fft_sink::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    if(noutput_items < 1)
    {
        struct timespec time_to_sleep = {0, 100000L };
        nanosleep(&time_to_sleep, NULL);
        return noutput_items;
    }

    gr_complex *in = (gr_complex*)(input_items[0]);
    memcpy(_fft->get_inbuf(), in, sizeof(gr_complex) * noutput_items);
    _fft->execute();
    unsigned int i;
    double gain;
    double pwr;
    std::complex<float> pt;             /* a single FFT point used in calculations */
    std::complex<float> scaleFactor;    /* normalizing factor (fftsize cast to complex) */
    scaleFactor = std::complex<float>((float)_fft_size);
    for (i = 0; i < _fft_size; i++)
    {

        /* normalize and shift */
        if (i < _fft_size/2)
        {
            pt = _fft->get_outbuf()[_fft_size/2+i] / scaleFactor;
        }
        else
        {
            pt = _fft->get_outbuf()[i-_fft_size/2] / scaleFactor;
        }
        pwr = pt.imag()*pt.imag() + pt.real()*pt.real();

        /* calculate power in dBFS */
        realFftData[i] = 10.0 * log10(pwr + 1.0e-20);

        /* FFT averaging (aka. video filter) */
        gain = d_fftAvg * (150.0+realFftData[i])/150.0;
        //gain = 0.1;

        iirFftData[i] = (1.0 - gain) * iirFftData[i] + gain * realFftData[i];

    }
    gr::thread::scoped_lock guard(_mutex);
    for(int i=0;i < sizeof(gr_complex) * _fft_size;i++)
    {
        _data->push_back(_fft->get_outbuf()[i]);
    }

    return noutput_items;
}
