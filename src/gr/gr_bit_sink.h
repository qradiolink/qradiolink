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


#ifndef GR_BIT_SINK_H
#define GR_BIT_SINK_H

#include <gnuradio/sync_block.h>
#include <gnuradio/sync_interpolator.h>
#include <gnuradio/io_signature.h>


class gr_bit_sink;
typedef std::shared_ptr<gr_bit_sink> gr_bit_sink_sptr;

gr_bit_sink_sptr make_gr_bit_sink();

class gr_bit_sink : public gr::sync_block
{
public:
    gr_bit_sink();
    ~gr_bit_sink();
    int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    std::vector<unsigned char>* get_data();
    void flush();

private:
    std::vector<unsigned char> *_data;
    gr::thread::condition_variable _cond_wait;
    gr::thread::mutex _mutex;
};

#endif // GR_BIT_SINK_H
