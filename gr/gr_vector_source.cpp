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

#include "gr_vector_source.h"


gr_vector_source_sptr
make_gr_vector_source ()
{
    return gnuradio::get_initial_sptr(new gr_vector_source);
}

gr_vector_source::gr_vector_source() :
        gr::sync_block("gr_vector_source",
                       gr::io_signature::make (0, 0, 0),
                       gr::io_signature::make (1, 1, sizeof (unsigned char)))
{
    _offset = 0;
    _finished = true;
    _data = new std::vector<unsigned char>;

}

gr_vector_source::~gr_vector_source()
{
    delete _data;
}


void gr_vector_source::flush()
{
    gr::thread::scoped_lock guard(_mutex);
    _data->clear();
}

int gr_vector_source::set_data(std::vector<unsigned char> *data)
{

    if(_offset == 0)
    {
        gr::thread::scoped_lock guard(_mutex);
        _data->insert(_data->end(),data->begin(),data->end());
        delete data;
        _finished = false;
        return 0;
    }
    else
    {
        return 1;
    }
}

int gr_vector_source::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) input_items;
    gr::thread::scoped_lock guard(_mutex);
    //if(_finished || (_data->size()==0))
    if(_finished)
    {
        guard.unlock();
        struct timespec time_to_sleep = {0, 35000000L };
        nanosleep(&time_to_sleep, NULL);
        _finished = false;
        return 0;
    }


    unsigned char *out = (unsigned char*)(output_items[0]);
    unsigned n = std::min((unsigned)_data->size() - _offset,
                                  (unsigned)noutput_items);
    for(unsigned i=0;i < n;i++)
    {
        out[i] = _data->at(_offset + i);
    }

    _offset += n;
    if(_offset == _data->size())
    {
        //gr::thread::scoped_lock guard(_mutex);
        _data->clear();
        _finished = true;
        _offset = 0;
    }
    return n;
}
