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


#include "gr_audio_sink.h"

gr_audio_sink_sptr
make_gr_audio_sink ()
{
    return gnuradio::get_initial_sptr(new gr_audio_sink);
}

gr_audio_sink::gr_audio_sink() :
        gr::sync_block("gr_audio_sink",
                       gr::io_signature::make (1, 1, sizeof (float)),
                       gr::io_signature::make (0, 0, 0))
{
    _offset = 0;
    _finished = false;
    _data = new std::vector<float>;

}

gr_audio_sink::~gr_audio_sink()
{
    delete _data;
}

void gr_audio_sink::flush()
{
    gr::thread::scoped_lock guard(_mutex);
    _data->clear();
}

std::vector<float> *gr_audio_sink::get_data()
{
    gr::thread::scoped_lock guard(_mutex);

    std::vector<float>* data = new std::vector<float>;
    // Have at least 40 ms of audio buffered
    if(_data->size() < 320)
    {
        return data;
    }
    data->reserve(_data->size());
    data->insert(data->end(),_data->begin(),_data->end());
    _data->clear();

    return data;
}

int gr_audio_sink::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) output_items;
    if(noutput_items < 1)
    {
        return noutput_items;
    }
    gr::thread::scoped_lock guard(_mutex);
    float *in = (float*)(input_items[0]);
    if(_data->size() > 320 * 10)
    {
        // not reading data fast enough, anything more than 400 msec
        // of data in the buffer is a problem downstream so dropping buffer
        return noutput_items;
    }
    for(int i=0;i < noutput_items;i++)
    {
        _data->push_back(in[i]);
    }

    return noutput_items;
}
