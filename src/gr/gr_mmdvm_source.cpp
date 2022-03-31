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

#include <QDebug>
#include "gr_mmdvm_source.h"
#include "src/bursttimer.h"
#define RPI
#include <Globals.h>


gr_mmdvm_source_sptr
make_gr_mmdvm_source ()
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_source);
}

static const pmt::pmt_t TIME_TAG = pmt::string_to_symbol("tx_time");
static const pmt::pmt_t LENGTH_TAG = pmt::string_to_symbol("burst_length");

gr_mmdvm_source::gr_mmdvm_source() :
        gr::sync_block("gr_mmdvm_source",
                       gr::io_signature::make (0, 0, 0),
                       gr::io_signature::make (1, 1, sizeof (short)))
{
    _offset = 0;
    _finished = true;
    _samp_rate = 1000000;
    //set_output_multiple(720);
}

gr_mmdvm_source::~gr_mmdvm_source()
{
}



int gr_mmdvm_source::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) input_items;
    short *out = (short*)(output_items[0]);
    ::pthread_mutex_lock(&m_TXlock);
    unsigned int buf_size = m_txBuffer.getData();
    if(buf_size < 1)
    {
        ::pthread_mutex_unlock(&m_TXlock);
        struct timespec time_to_sleep = {0, 10000L };
        nanosleep(&time_to_sleep, NULL);
        return 0;
    }
    unsigned int n = std::min((unsigned int)buf_size,
                                  (unsigned int)noutput_items);
    for(unsigned int i = 0;i < n; i++)
    {
        uint16_t sample = 0;
        uint8_t control = MARK_NONE;
        m_txBuffer.get(sample, control);
        sample *= 5;		// amplify by 12dB
        short signed_sample = (short)sample;
        out[i] = signed_sample;
        bool add_tag = false;
        if(control == MARK_SLOT1)
        {
            uint64_t time = burst_timer.allocate_slot(1, add_tag);
            if(add_tag)
                add_time_tag(time, i);
        }
        else if(control == MARK_SLOT2)
        {
            uint64_t time = burst_timer.allocate_slot(2, add_tag);
            if(add_tag)
                add_time_tag(time, i);
        }
    }
    ::pthread_mutex_unlock(&m_TXlock);

    return n;
}

void gr_mmdvm_source::set_samp_rate(int samp_rate)
{
    _samp_rate = (double)samp_rate;
}

// Add rx_time tag to stream
void gr_mmdvm_source::add_time_tag(uint64_t usec, int offset) {

    uint64_t intpart = usec / 1000000000;
    double fracpart = ((double)usec / 1000000000.0d) - (double)intpart;

    const pmt::pmt_t t_val = pmt::make_tuple(pmt::from_uint64(intpart), pmt::from_double(fracpart));
    const pmt::pmt_t b_val = pmt::from_long(30000);
    //qDebug() << "Intpart: " << intpart << " Fracpart: " << fracpart << " Usec: " << usec;
    this->add_item_tag(0, nitems_written(0) + offset, TIME_TAG, t_val);
    //this->add_item_tag(0, nitems_written(0) + offset, LENGTH_TAG, b_val);
}

