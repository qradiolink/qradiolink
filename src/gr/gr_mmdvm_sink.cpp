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

#include "gr_mmdvm_sink.h"

const uint8_t  MARK_SLOT1 = 0x08U;
const uint8_t  MARK_SLOT2 = 0x04U;
const uint8_t  MARK_NONE  = 0x00U;

gr_mmdvm_sink_sptr
make_gr_mmdvm_sink (BurstTimer *burst_timer, uint8_t cn, bool multi_channel)
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_sink(burst_timer, cn, multi_channel));
}

static const pmt::pmt_t TIME_TAG = pmt::string_to_symbol("rx_time");

gr_mmdvm_sink::gr_mmdvm_sink(BurstTimer *burst_timer, uint8_t cn, bool multi_channel) :
        gr::sync_block("gr_mmdvm_sink",
                       gr::io_signature::make (1, 1, sizeof (short)),
                       gr::io_signature::make (0, 0, 0))
{
    _burst_timer = burst_timer;
    _channel_number = cn - 1;
    _zmqcontext = zmq::context_t(1);
    _zmqsocket = zmq::socket_t(_zmqcontext, ZMQ_PUSH);
    _zmqsocket.setsockopt(ZMQ_SNDHWM, 2);
    _zmqsocket.setsockopt(ZMQ_LINGER, 0);
    int socket_no = multi_channel ? cn : 0;
    _zmqsocket.bind ("ipc:///tmp/mmdvm-rx" + std::to_string(socket_no) + ".ipc");

    set_min_noutput_items(SAMPLES_PER_SLOT);
    set_max_noutput_items(SAMPLES_PER_SLOT);
}

gr_mmdvm_sink::~gr_mmdvm_sink()
{

}

int gr_mmdvm_sink::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) output_items;
    short *in = (short*)(input_items[0]);
    t1 = std::chrono::high_resolution_clock::now();
    std::vector<gr::tag_t> tags;
    uint64_t nitems = nitems_read(0);
    std::vector<uint8_t> control_buf;
    std::vector<int16_t> data_buf;


    get_tags_in_window(tags, 0, 0, noutput_items, TIME_TAG);
    if (!tags.empty()) {

        std::sort(tags.begin(), tags.end(), gr::tag_t::offset_compare);
    }

    for(int i = 0;i < noutput_items; i++)
    {
        bool time_base_received = false;
        for (gr::tag_t tag : tags)
        {
            if(tag.offset == nitems + (uint64_t)i)
            {
                uint64_t secs = pmt::to_uint64(pmt::tuple_ref(tag.value, 0));
                double fracs = pmt::to_double(pmt::tuple_ref(tag.value, 1));
                // nanoseconds
                uint64_t time = uint64_t(llround(double(secs * 1000000000L) + (fracs * 1000000000.0d)));
                _burst_timer->set_timer(time, _channel_number);
                time_base_received = true;
                break;
            }
        }
        uint8_t control = MARK_NONE;
        int slot_no = _burst_timer->check_time(_channel_number);

        if(slot_no == 1)
        {
            control = MARK_SLOT1;
        }
        if(slot_no == 2)
        {
            control = MARK_SLOT2;
        }
        control_buf.push_back(control);
        data_buf.push_back((int16_t)in[i]);
        if(!time_base_received)
            _burst_timer->increment_sample_counter(_channel_number);

    }
    uint32_t num_items = (uint32_t)noutput_items;
    int buf_size = sizeof(uint32_t) + num_items * sizeof(uint8_t) + num_items * sizeof(int16_t);
    zmq::message_t reply (buf_size);
    memcpy (reply.data (), &num_items, sizeof(uint32_t));
    memcpy ((unsigned char *)reply.data () + sizeof(uint32_t), (unsigned char *)control_buf.data(), num_items * sizeof(uint8_t));
    memcpy ((unsigned char *)reply.data () + sizeof(uint32_t) + num_items * sizeof(uint8_t),
            (unsigned char *)data_buf.data(), num_items*sizeof(int16_t));
    _zmqsocket.send (reply, zmq::send_flags::dontwait);

    return noutput_items;
}
