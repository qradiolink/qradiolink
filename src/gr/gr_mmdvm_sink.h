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

#ifndef GR_MMDV_SINK_H
#define GR_MMDV_SINK_H

#include <gnuradio/sync_block.h>
#include <gnuradio/sync_interpolator.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/tags.h>
#include <zmq.hpp>
#include "src/bursttimer.h"



class gr_mmdvm_sink;
typedef std::shared_ptr<gr_mmdvm_sink> gr_mmdvm_sink_sptr;

gr_mmdvm_sink_sptr make_gr_mmdvm_sink(BurstTimer *burst_timer, uint8_t cn=0,
                                      bool multi_channel=true, bool use_tdma=true);

class gr_mmdvm_sink : public gr::sync_block
{
public:
    gr_mmdvm_sink(BurstTimer *burst_timer, uint8_t cn=0, bool multi_channel=true, bool use_tdma=true);
    ~gr_mmdvm_sink();
    int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);


private:
    BurstTimer *_burst_timer;
    zmq::context_t _zmqcontext[MAX_MMDVM_CHANNELS];
    zmq::socket_t _zmqsocket[MAX_MMDVM_CHANNELS];
    std::vector<uint8_t> control_buf[MAX_MMDVM_CHANNELS];
    std::vector<int16_t> data_buf[MAX_MMDVM_CHANNELS];
    int _num_channels;
    bool _use_tdma;
};


#endif // GR_MMDV_SINK_H
