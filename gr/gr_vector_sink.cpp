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


#include "gr_vector_sink.h"

gr_vector_sink_sptr
make_gr_vector_sink ()
{
    return gnuradio::get_initial_sptr(new gr_vector_sink);
}

gr_vector_sink::gr_vector_sink() :
        gr::sync_block("gr_vector_sink",
                       gr::io_signature::make (1, 1, sizeof (unsigned char)),
                       gr::io_signature::make (0, 0, 0))
{
    _offset = 0;
    _finished = false;
    _data = new std::vector<unsigned char>;

}

gr_vector_sink::~gr_vector_sink()
{
    delete _data;
}

void gr_vector_sink::flush()
{
    gr::thread::scoped_lock guard(_mutex);
    _data->clear();
}

std::vector<unsigned char> * gr_vector_sink::get_data()
{
    gr::thread::scoped_lock guard(_mutex);
    std::vector<unsigned char>* data = new std::vector<unsigned char>;
    // Buffer up to X bits
    if(_data->size() < 1)
    {
        return data;
    }
    data->reserve(_data->size());
    data->insert(data->end(),_data->begin(),_data->end());
    _data->clear();

    return data;
}

int gr_vector_sink::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) output_items;
    if(noutput_items < 1)
    {
        return noutput_items;
    }
    gr::thread::scoped_lock guard(_mutex);
    if(_data->size() > 1024 * 1024)
    {
        // not reading data fast enough, anything more than 400 msec
        // of data in the buffer is a problem downstream so dropping buffer
        return noutput_items;
    }
    unsigned char *in = (unsigned char*)(input_items[0]);
    for(int i=0;i < noutput_items;i++)
    {
        _data->push_back(in[i]);
    }

    return noutput_items;
}
