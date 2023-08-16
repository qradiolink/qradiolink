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

#include "gr_byte_source.h"


gr_byte_source_sptr
make_gr_byte_source ()
{
    return gnuradio::get_initial_sptr(new gr_byte_source);
}

gr_byte_source::gr_byte_source() :
        gr::sync_block("gr_byte_source",
                       gr::io_signature::make (0, 0, 0),
                       gr::io_signature::make (1, 1, sizeof (unsigned char)))
{
    _offset = 0;
    _finished = true;
    _data = new std::vector<unsigned char>;
}

gr_byte_source::~gr_byte_source()
{
    delete _data;
}


void gr_byte_source::flush()
{
    gr::thread::scoped_lock guard(_mutex);
    _data->clear();
}

int gr_byte_source::set_data(std::vector<unsigned char> *data)
{

    gr::thread::scoped_lock guard(_mutex);
    _data->reserve(_data->size() + data->size());
    _data->insert(_data->end(),data->begin(),data->end());
    data->clear();
    delete data;
    _finished = false;
    return 0;
}

int gr_byte_source::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) input_items;
    gr::thread::scoped_lock guard(_mutex);
    if(_finished)
    {
        _finished = false;
        guard.unlock();
        struct timespec time_to_sleep = {0, 5000000L };
        nanosleep(&time_to_sleep, NULL);
        return 0;
    }


    unsigned char *out = (unsigned char*)(output_items[0]);
    unsigned int n = std::min((unsigned int)_data->size() - _offset,
                                  (unsigned int)noutput_items);
    for(unsigned int i=0;i < n;i++)
    {
        out[i] = _data->at(i);
    }

    if(_data->size() > 0)
        _data->erase(_data->begin(), _data->begin() + n);
    if(_data->size() < 1)
    {
        _finished = true;
    }
    return n;
}
