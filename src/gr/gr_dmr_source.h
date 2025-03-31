// Written by Adrian Musceac YO8RZZ , started October 2024.
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

#ifndef GR_DMR_SOURCE_H
#define GR_DMR_SOURCE_H


#include <gnuradio/sync_block.h>
#include <gnuradio/sync_interpolator.h>
#include <gnuradio/io_signature.h>
#include "src/DMR/dmrtiming.h"
#include "src/DMR/dmrframe.h"


class gr_dmr_source;

typedef std::shared_ptr<gr_dmr_source> gr_dmr_source_sptr;

gr_dmr_source_sptr make_gr_dmr_source(DMRTiming *dmrtiming);

class gr_dmr_source : public gr::sync_block
{
public:
    gr_dmr_source(DMRTiming *dmrtiming);
    ~gr_dmr_source();

    int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    int set_data(std::vector<DMRFrame> &frames);
    void add_time_tag(uint64_t nsec, int offset, int which);
    void add_zero_tag(int offset, uint64_t num_samples, int which);
    void flush();
private:
    DMRTiming *_dmr_timing;
    std::vector<std::vector<uint8_t>> _frame_buffer;
    std::vector<uint8_t> _slot_numbers;
    gr::thread::mutex _mutex;
    void message_handler_function(const pmt::pmt_t& msg);
    uint64_t _init_counter;
};

#endif // GR_DMR_SOURCE_H
