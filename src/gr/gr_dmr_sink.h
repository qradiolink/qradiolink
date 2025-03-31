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


#ifndef GR_DMR_SINK_H
#define GR_DMR_SINK_H

#include <gnuradio/sync_block.h>
#include <gnuradio/sync_interpolator.h>
#include <gnuradio/io_signature.h>
#include "src/DMR/dmrframe.h"
#include "src/DMR/dmrtiming.h"



class gr_dmr_sink;
typedef std::shared_ptr<gr_dmr_sink> gr_dmr_sink_sptr;

gr_dmr_sink_sptr make_gr_dmr_sink(DMRTiming *dmrtiming);

class gr_dmr_sink : public gr::sync_block
{
public:
    gr_dmr_sink(DMRTiming *dmrtiming);
    ~gr_dmr_sink();
    int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    std::vector<DMRFrame> get_data();
    void flush();

private:
    bool findSync(uint8_t bit, uint8_t ts_index);
    uint8_t countSyncErrs(uint64_t sync);
    bool processBits(uint8_t bit, uint8_t ts_index);
    uint8_t nextSlot(uint8_t ts_index);
    void nextBit();
    std::vector<uint8_t> _bit_buffer[2];
    std::vector<DMRFrame> _frame_buffer;
    gr::thread::condition_variable _cond_wait;
    gr::thread::mutex _mutex;
    unsigned int _state[2];
    uint64_t _shift_register[2];
    uint32_t _bits_to_receive[2];
    uint32_t _frames_to_receive[2];
    bool _downlink[2];
    uint8_t _next_slot;
    uint64_t _slot_sample_counter[2];
    uint8_t _symbol_bits;
    DMRTiming *_dmr_timing;
};

#endif // GR_DMR_SINK_H
