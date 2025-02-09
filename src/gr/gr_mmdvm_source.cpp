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
const int32_t  ZERO_SAMPLES = SAMPLES_PER_SLOT * 25 / 24; // resampling ratio

gr_mmdvm_source_sptr
make_gr_mmdvm_source (BurstTimer *burst_timer, uint8_t cn, bool multi_channnel, bool use_tdma)
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_source(burst_timer, cn, multi_channnel, use_tdma));
}

static const pmt::pmt_t TIME_TAG = pmt::string_to_symbol("tx_time");
static const pmt::pmt_t LENGTH_TAG = pmt::string_to_symbol("burst_length");
static const pmt::pmt_t ZERO_TAG = pmt::string_to_symbol("zero_samples");

gr_mmdvm_source::gr_mmdvm_source(BurstTimer *burst_timer, uint8_t cn, bool multi_channel, bool use_tdma) :
        gr::sync_block("gr_mmdvm_source",
                       gr::io_signature::make (0, 0, 0),
                       gr::io_signature::make (cn, cn, sizeof (short)))
{
    _burst_timer = burst_timer;
    _num_channels = cn;
    _sn = 2;
    _timing_correction = 0;
    _add_time_tag = (cn == 0) || (cn == 1);
    _use_tdma = use_tdma;
    for(int i = 0;i < _num_channels;i++)
    {
        _zmqcontext[i] = zmq::context_t(1);
        _zmqsocket[i] = zmq::socket_t(_zmqcontext[i], ZMQ_REQ);
        _zmqsocket[i].setsockopt(ZMQ_SNDHWM, 10);
        _zmqsocket[i].setsockopt(ZMQ_LINGER, 0);
        int socket_no = multi_channel ? i + 1 : i;
        _zmqsocket[i].connect ("ipc:///tmp/mmdvm-tx" + std::to_string(socket_no) + ".ipc");
        _in_tx[i] = false;
    }
    set_min_noutput_items(SAMPLES_PER_SLOT);
    set_max_noutput_items(SAMPLES_PER_SLOT);
}

gr_mmdvm_source::~gr_mmdvm_source()
{
}


void gr_mmdvm_source::get_zmq_message()
{

    for(int j = 0;j < _num_channels;j++)
    {
        zmq::message_t mq_message;
        int size = 0;
        zmq::recv_result_t recv_result;
        //std::chrono::high_resolution_clock::time_point tw1 = std::chrono::high_resolution_clock::now();
        zmq::message_t request_msg (1);
        memcpy (request_msg.data (), "s", sizeof(char));
        _zmqsocket[j].send (request_msg, zmq::send_flags::none);
        recv_result = _zmqsocket[j].recv(mq_message);
        size = mq_message.size();
        //std::chrono::high_resolution_clock::time_point tw2 = std::chrono::high_resolution_clock::now();
        //int64_t wait_time = std::chrono::duration_cast<std::chrono::nanoseconds>(tw2-tw1).count();
        if(size < 1)
        {
            _in_tx[j] = false;
            continue;
        }

        uint32_t buf_size = 0;
        memcpy(&buf_size, (uint8_t*)mq_message.data(), sizeof(uint32_t));

        if(buf_size > 0)
        {
            _in_tx[j] = true;
            uint8_t control[buf_size];
            int16_t data[buf_size];
            memcpy(&control, (uint8_t*)mq_message.data() + sizeof(uint32_t), buf_size * sizeof(uint8_t));

            memcpy(&data, (uint8_t*)mq_message.data() + sizeof(uint32_t) + buf_size * sizeof(uint8_t),
                   buf_size * sizeof(int16_t));
            for(uint32_t i=0;i<buf_size;i++)
            {
                control_buf[j].push_back(control[i]);
                data_buf[j].push_back(data[i]);
            }
        }
        else
        {
            _in_tx[j] = false;
        }
    }

}

void gr_mmdvm_source::handle_idle_time(short *out, int noutput_items, int which, bool add_tag)
{
    alternate_slots();
    add_zero_tag(0, ZERO_SAMPLES, which);
    for(int i = 0;i < noutput_items; i++)
    {
        out[i] = 0;
        if(i == 710)
        {
            uint64_t time = _burst_timer->allocate_slot(_sn, _timing_correction, which);
            if(time > 0L && add_tag)
            {
                add_time_tag(time, i, which);
            }
        }
    }
}

int gr_mmdvm_source::handle_data_bursts(short *out, unsigned int n, int which, bool add_tag)
{
    int num_tags_added = 0;
    for(unsigned int i = 0;i < n; i++)
    {
        uint8_t control = control_buf[which].at(i);
        if(control == MARK_SLOT1 || control == MARK_SLOT2)
        {
            num_tags_added++;
        }
    }
    //if(num_tags_added < 1)
    //    return 0;

    for(unsigned int i = 0;i < n; i++)
    {
        short sample = data_buf[which].at(i);
        uint8_t control = control_buf[which].at(i);
        out[i] = sample;
        if(control == MARK_SLOT1)
        {
            _sn = 1;
            uint64_t time = _burst_timer->allocate_slot(1, _timing_correction, which);
            if(time > 0L && add_tag)
            {
                add_time_tag(time, i, which);
            }
        }
        if(control == MARK_SLOT2)
        {
            _sn = 2;
            uint64_t time = _burst_timer->allocate_slot(2, _timing_correction, which);
            if(time > 0 && add_tag)
            {
                add_time_tag(time, i, which);
            }
        }
    }
    return num_tags_added;
}

void gr_mmdvm_source::alternate_slots()
{
    if(_sn == 2)
        _sn = 1;
    else
        _sn = 2;
}

int gr_mmdvm_source::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) input_items;
    short *out[MAX_MMDVM_CHANNELS];
    for(int i = 0;i < _num_channels;i++)
    {
        out[i] = (short*)(output_items[i]);
    }

    bool start = true;
    for(int i = 0;i < _num_channels;i++)
    {
        if(_burst_timer->get_sample_counter(i) < 1000)
        {
            std::cout << "Waiting for RX samples to initialize timebase" << std::endl;
            control_buf[i].clear();
            data_buf[i].clear();
            start = false;
        }
    }
    if(!start && _use_tdma)
        return 0;
    else if(!start)
    {
        return SAMPLES_PER_SLOT;
    }
    get_zmq_message();
    if(_timing_correction > 0)
    {
        struct timespec time_to_sleep = {0, _timing_correction};
        nanosleep(&time_to_sleep, NULL);
        _timing_correction = 0;
    }

    for(int i = 0;i < _num_channels;i++)
    {
        if(data_buf[i].size() < 1)
        {
            handle_idle_time(out[i], noutput_items, i, i == 0);
        }
    }
    for(int i = 0;i < _num_channels;i++)
    {
        unsigned int n = std::min((unsigned int)data_buf[i].size(),
                                      (unsigned int)noutput_items);

        int num_tags_added = handle_data_bursts(out[i], n, i, i == 0);
        /*
        if(num_tags_added < 1)
        {
            data_buf.erase(data_buf.begin(), data_buf.begin() + n);
            control_buf.erase(control_buf.begin(), control_buf.begin() + n);
            std::cerr << "Found DMR burst with zero timeslot marks" << std::endl;
            handle_idle_time(timing_adjust, out, noutput_items);
            return noutput_items;
        }
        */
        data_buf[i].erase(data_buf[i].begin(), data_buf[i].begin() + n);
        control_buf[i].erase(control_buf[i].begin(), control_buf[i].begin() + n);
    }
    return SAMPLES_PER_SLOT;
}


// Add tx_time tag to stream
void gr_mmdvm_source::add_time_tag(uint64_t nsec, int offset, int which)
{
    uint64_t intpart = nsec / 1000000000L;
    double fracpart = ((double)nsec / 1000000000.0d) - (double)intpart;

    const pmt::pmt_t t_val = pmt::make_tuple(pmt::from_uint64(intpart), pmt::from_double(fracpart));
    this->add_item_tag(which, nitems_written(which) + (uint64_t)offset, TIME_TAG, t_val);
    /// length tag doesn't seem to be necessary
    const pmt::pmt_t b_val = pmt::from_long(SAMPLES_PER_SLOT);
    this->add_item_tag(0, nitems_written(0) + offset, LENGTH_TAG, b_val);

}

// Add zero samples tag to stream
void gr_mmdvm_source::add_zero_tag(int offset, int num_samples, int which)
{
    const pmt::pmt_t t_val = pmt::from_uint64((uint64_t)num_samples);
    this->add_item_tag(which, nitems_written(which) + (uint64_t)offset, ZERO_TAG, t_val);
}

