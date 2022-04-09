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
#include <QDebug>


gr_mmdvm_sink_sptr
make_gr_mmdvm_sink (BurstTimer *burst_timer)
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_sink(burst_timer));
}

static const pmt::pmt_t TIME_TAG = pmt::string_to_symbol("rx_time");

gr_mmdvm_sink::gr_mmdvm_sink(BurstTimer *burst_timer) :
        gr::block("gr_mmdvm_sink",
                       gr::io_signature::make (1, 1, sizeof (short)),
                       gr::io_signature::make (0, 0, 0))
{
    set_tag_propagation_policy(TPP_ALL_TO_ALL);
    _burst_timer = burst_timer;
    _zmqcontext = zmq::context_t(1);
    _zmqsocket = zmq::socket_t(_zmqcontext, ZMQ_PUSH);
    _zmqsocket.bind ("ipc:///tmp/mmdvm-rx.ipc");
}

gr_mmdvm_sink::~gr_mmdvm_sink()
{

}



int gr_mmdvm_sink::general_work(int noutput_items, gr_vector_int &ninput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) output_items;
    short *in = (short*)(input_items[0]);
    std::vector<gr::tag_t> tags;
    uint64_t nitems = nitems_read(0);


    get_tags_in_window(tags, 0, 0, noutput_items, TIME_TAG);
    if (!tags.empty()) {

        std::sort(tags.begin(), tags.end(), gr::tag_t::offset_compare);
    }

    ::pthread_mutex_lock(&m_RXlock);
    for(int i = 0;i < noutput_items; i++)
    {
        for (gr::tag_t tag : tags)
        {
            if(tag.offset == nitems + (uint64_t)i)
            {
                uint64_t secs = pmt::to_uint64(pmt::tuple_ref(tag.value, 0));
                double fracs = pmt::to_double(pmt::tuple_ref(tag.value, 1));
                // nanoseconds
                uint64_t time = uint64_t((double)secs * 1000000000.0d) + uint64_t(fracs * 1000000000.0d);
                _burst_timer->set_timer(time);
            }
        }
        uint8_t control = MARK_NONE;
        int slot_no = _burst_timer->check_time();

        if(slot_no == 1)
        {
            control = MARK_SLOT1;
        }
        if(slot_no == 2)
        {
            control = MARK_SLOT2;
        }
        m_rxBuffer.put((uint16_t)in[i], control);
        _burst_timer->increment_sample_counter();

    }
    ::pthread_mutex_unlock(&m_RXlock);


    consume(0, noutput_items);
    return noutput_items;
}
