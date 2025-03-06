// Written by Adrian Musceac YO8RZZ , started March 2021.
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

#ifndef GR_ZERO_IDLE_BURSTS_H
#define GR_ZERO_IDLE_BURSTS_H

#include <gnuradio/sync_block.h>
#include <gnuradio/sync_interpolator.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/tags.h>
#include <src/bursttimer.h>

class gr_zero_idle_bursts;
typedef std::shared_ptr<gr_zero_idle_bursts> gr_zero_idle_bursts_sptr;

gr_zero_idle_bursts_sptr make_gr_zero_idle_bursts(unsigned int delay=0);

class gr_zero_idle_bursts : public gr::sync_block
{
public:
    gr_zero_idle_bursts(unsigned int delay=0);
    ~gr_zero_idle_bursts();
    int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

private:
    uint64_t _sample_counter;
    unsigned int _delay;
};

#endif // GR_ZERO_IDLE_BURSTS_H
