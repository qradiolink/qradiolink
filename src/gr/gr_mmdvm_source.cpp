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

#include <QDebug>
#include "gr_mmdvm_source.h"

const uint8_t  MARK_SLOT1 = 0x08U;
const uint8_t  MARK_SLOT2 = 0x04U;
const uint8_t  MARK_NONE  = 0x00U;

gr_mmdvm_source_sptr
make_gr_mmdvm_source (BurstTimer *burst_timer)
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_source(burst_timer));
}

static const pmt::pmt_t TIME_TAG = pmt::string_to_symbol("tx_time");
static const pmt::pmt_t LENGTH_TAG = pmt::string_to_symbol("burst_length");

gr_mmdvm_source::gr_mmdvm_source(BurstTimer *burst_timer) :
        gr::sync_block("gr_mmdvm_source",
                       gr::io_signature::make (0, 0, 0),
                       gr::io_signature::make (1, 1, sizeof (short)))
{
    _offset = 0;
    _finished = true;
    _samp_rate = 1000000;
    _burst_timer = burst_timer;
    _zmqcontext = zmq::context_t(1);
    _zmqsocket = zmq::socket_t(_zmqcontext, ZMQ_PULL);
    _zmqsocket.connect ("ipc:///tmp/mmdvm-tx.ipc");
}

gr_mmdvm_source::~gr_mmdvm_source()
{
}

int gr_mmdvm_source::get_zmq_message()
{
    zmq::message_t mq_message;
    zmq::recv_result_t recv_result = _zmqsocket.recv(mq_message, zmq::recv_flags::dontwait);
    int size = mq_message.size();
    if(size < 1)
        return 0;
    uint32_t buf_size = 0;
    memcpy(&buf_size, (uint8_t*)mq_message.data(), sizeof(uint32_t));
    uint8_t control[buf_size];
    int16_t data[buf_size];
    if(buf_size > 0)
    {

        memcpy(&control, (uint8_t*)mq_message.data() + sizeof(uint32_t), buf_size * sizeof(uint8_t));

        memcpy(&data, (uint8_t*)mq_message.data() + sizeof(uint32_t) + buf_size * sizeof(uint8_t),
               buf_size * sizeof(int16_t));
    }
    for(uint32_t i=0;i<buf_size;i++)
    {
        control_buf.push_back(control[i]);
        data_buf.push_back(data[i]);
    }
    return buf_size;
}

int gr_mmdvm_source::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) input_items;
    short *out = (short*)(output_items[0]);

    get_zmq_message();
    if(data_buf.size() < 1)
    {
        //struct timespec time_to_sleep = {0, 10000L };
        //nanosleep(&time_to_sleep, NULL);
        return 0;
    }
    unsigned int n = std::min((unsigned int)data_buf.size(),
                                  (unsigned int)noutput_items);

    for(unsigned int i = 0;i < n; i++)
    {
        short sample = data_buf.at(i);
        uint8_t control = control_buf.at(i);
        out[i] = sample;
        if(control == MARK_SLOT1)
        {
            uint64_t time = _burst_timer->allocate_slot(1);
            add_time_tag(time, i);
        }
        if(control == MARK_SLOT2)
        {
            uint64_t time = _burst_timer->allocate_slot(2);
            add_time_tag(time, i);
        }
    }
    data_buf.erase(data_buf.begin(), data_buf.begin() + n);
    control_buf.erase(control_buf.begin(), control_buf.begin() + n);
    return n;
}

void gr_mmdvm_source::set_samp_rate(int samp_rate)
{
    _samp_rate = (double)samp_rate;
}

// Add rx_time tag to stream
void gr_mmdvm_source::add_time_tag(uint64_t nsec, int offset)
{
    uint64_t intpart = nsec / 1000000000L;
    double fracpart = ((double)nsec / 1000000000.0d) - (double)intpart;

    const pmt::pmt_t t_val = pmt::make_tuple(pmt::from_uint64(intpart), pmt::from_double(fracpart));
    this->add_item_tag(0, nitems_written(0) + (uint64_t)offset, TIME_TAG, t_val);
    /// length tag doesn't seem to be necessary
    //const pmt::pmt_t b_val = pmt::from_long(30000);
    //this->add_item_tag(0, nitems_written(0) + offset, LENGTH_TAG, b_val);

}

