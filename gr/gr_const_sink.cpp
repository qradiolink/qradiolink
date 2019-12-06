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


#include "gr_const_sink.h"

gr_const_sink_sptr
make_gr_const_sink ()
{
    return gnuradio::get_initial_sptr(new gr_const_sink);
}

gr_const_sink::gr_const_sink() :
        gr::sync_block("gr_const_sink",
                       gr::io_signature::make (1, 1, sizeof (gr_complex)),
                       gr::io_signature::make (0, 0, 0))
{
    _offset = 0;
    _finished = false;
    _data = new std::vector<gr_complex>;

}

gr_const_sink::~gr_const_sink()
{
    delete _data;
}

void gr_const_sink::flush()
{
    gr::thread::scoped_lock guard(_mutex);
    _data->clear();
}

std::vector<gr_complex> *gr_const_sink::get_data()
{
    std::vector<gr_complex>* data = new std::vector<gr_complex>;
    gr::thread::scoped_lock guard(_mutex);

    if(_data->size() < 32)
    {
        return data;
    }
    data->reserve(_data->size());
    data->insert(data->end(),_data->begin(),_data->end());
    _data->clear();

    return data;
}

int gr_const_sink::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) output_items;
    gr_complex *in = (gr_complex*)(input_items[0]);
    if(noutput_items < 1)
    {
        return noutput_items;
    }
    if(_data->size() > 256)
    {
        return noutput_items;
    }
    gr::thread::scoped_lock guard(_mutex);

    for(int i=0;i < noutput_items;i++)
    {
        _data->push_back(in[i]);
    }

    return noutput_items;
}
