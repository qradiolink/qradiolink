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
#include <math.h>

const uint8_t  MARK_SLOT1 = 0x08U;
const uint8_t  MARK_SLOT2 = 0x04U;
const uint8_t  MARK_NONE  = 0x00U;

gr_mmdvm_sink_sptr
make_gr_mmdvm_sink (BurstTimer *burst_timer, uint8_t cn, bool multi_channel, bool use_tdma)
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_sink(burst_timer, cn, multi_channel, use_tdma));
}

static const pmt::pmt_t TIME_TAG = pmt::string_to_symbol("rx_time");
static const pmt::pmt_t RSSI_TAG = pmt::string_to_symbol("RSSI");

gr_mmdvm_sink::gr_mmdvm_sink(BurstTimer *burst_timer, uint8_t cn, bool multi_channel, bool use_tdma) :
        gr::sync_block("gr_mmdvm_sink",
                       gr::io_signature::make (cn, cn, sizeof (short)),
                       gr::io_signature::make (0, 0, 0))
{
    _burst_timer = burst_timer;
    _num_channels = cn;
    _use_tdma = use_tdma;

    for(int i = 0;i < _num_channels;i++)
    {
        _zmqcontext[i] = zmq::context_t(1);
        _zmqsocket[i] = zmq::socket_t(_zmqcontext[i], ZMQ_PUSH);
        _zmqsocket[i].set(zmq::sockopt::sndhwm, 100);
        _zmqsocket[i].set(zmq::sockopt::linger, 0);
        int socket_no = multi_channel ? i + 1 : 0;
        _zmqsocket[i].bind ("ipc:///tmp/mmdvm-rx" + std::to_string(socket_no) + ".ipc");
        _last_rssi_on_timeslot[i] = 0;
        _slot_sample_counter[i] = 0;
    }


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
    short *in[MAX_MMDVM_CHANNELS];
    for(int i = 0;i < _num_channels;i++)
    {
        in[i] = (short*)(input_items[i]);
    }

    for(int chan = 0;chan < _num_channels;chan++)
    {
        std::vector<gr::tag_t> tags;
        std::vector<gr::tag_t> rssi_tags;
        uint64_t nitems = nitems_read(chan);

        get_tags_in_window(tags, chan, 0, noutput_items, TIME_TAG);
        if (!tags.empty()) {

            std::sort(tags.begin(), tags.end(), gr::tag_t::offset_compare);
        }
        get_tags_in_window(rssi_tags, chan, 0, noutput_items, RSSI_TAG);
        if (!rssi_tags.empty()) {

            std::sort(rssi_tags.begin(), rssi_tags.end(), gr::tag_t::offset_compare);
        }
        for (gr::tag_t tag : rssi_tags)
        {
            uint32_t rssi = (uint32_t)fabs(pmt::to_float((tag.value)));
            _rssi[chan].push_back(rssi);
        }

        for(int i = 0;i < noutput_items; i++)
        {
            bool time_base_received = false;
            if(_slot_sample_counter[chan] > 0)
            {
                _slot_sample_counter[chan]++;
            }
            for (gr::tag_t tag : tags)
            {
                if(tag.offset == nitems + (uint64_t)i)
                {
                    uint64_t secs = pmt::to_uint64(pmt::tuple_ref(tag.value, 0));
                    double fracs = pmt::to_double(pmt::tuple_ref(tag.value, 1));
                    // nanoseconds
                    uint64_t time = uint64_t(llround(double(secs * 1000000000L) + (fracs * 1000000000.0d)));
                    _burst_timer->set_timer(time, chan);
                    time_base_received = true;
                    break;
                }
            }
            if(!time_base_received)
                _burst_timer->increment_sample_counter(chan);
            uint8_t control = MARK_NONE;
            int slot_no = _burst_timer->check_time(chan);

            if(slot_no == 1)
            {
                control = MARK_SLOT1;
                _slot_sample_counter[chan] = 1;
            }
            if(slot_no == 2)
            {
                control = MARK_SLOT2;
                _slot_sample_counter[chan] = 1;
            }

            control_buf[chan].push_back(control);
            data_buf[chan].push_back((int16_t)in[chan][i]);

            if(_slot_sample_counter[chan] >= SAMPLES_PER_SLOT)
            {
                uint32_t rssi1 = _rssi[chan].back();
                uint32_t rssi2 = 32767;
                if(_rssi[chan].size() > 1)
                {
                    _rssi[chan].pop_back();
                    rssi2 = _rssi[chan].back();
                }
                _last_rssi_on_timeslot[chan] = (rssi1 < rssi2) ? rssi1 : rssi2;
                _rssi[chan].clear();
                _slot_sample_counter[chan] = 0;
            }
        }
        // buffer up to two timeslots before sending samples to MMDVM
        // introduces a delay of minimum 30 mseconds
        if(data_buf[chan].size() >= SAMPLES_PER_SLOT)
        {
            uint32_t num_items = SAMPLES_PER_SLOT;
            int buf_size = 2 * sizeof(uint32_t) + num_items * sizeof(uint8_t) + num_items * sizeof(int16_t);
            zmq::message_t reply (buf_size);
            memcpy (reply.data (), &num_items, sizeof(uint32_t));
            memcpy ((unsigned char *)reply.data () + sizeof(uint32_t), &(_last_rssi_on_timeslot[chan]), sizeof(uint32_t));
            memcpy ((unsigned char *)reply.data () + 2 * sizeof(uint32_t), (unsigned char *)control_buf[chan].data(), num_items * sizeof(uint8_t));
            memcpy ((unsigned char *)reply.data () + 2 * sizeof(uint32_t) + num_items * sizeof(uint8_t),
                    (unsigned char *)data_buf[chan].data(), num_items*sizeof(int16_t));
            _zmqsocket[chan].send (reply, zmq::send_flags::dontwait);
            data_buf[chan].erase(data_buf[chan].begin(), data_buf[chan].begin() + num_items);
            control_buf[chan].erase(control_buf[chan].begin(), control_buf[chan].begin() + num_items);
            _last_rssi_on_timeslot[chan] = 0;

        }
    }

    return noutput_items;
}
