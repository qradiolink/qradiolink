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

#include "gr_mmdvm_source.h"
#define RPI
#include <Globals.h>


gr_mmdvm_source_sptr
make_gr_mmdvm_source ()
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_source);
}

gr_mmdvm_source::gr_mmdvm_source() :
        gr::sync_block("gr_mmdvm_source",
                       gr::io_signature::make (0, 0, 0),
                       gr::io_signature::make (1, 1, sizeof (short)))
{
    _offset = 0;
    _finished = true;
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
        //struct timespec time_to_sleep = {0, 5000000L };
        //nanosleep(&time_to_sleep, NULL);
        return 0;
    }
    unsigned int n = std::min((unsigned int)buf_size,
                                  (unsigned int)noutput_items);
    for(int i = 0;i < n; i++)
    {
        uint16_t sample = 0;
        uint8_t control = MARK_NONE;
        m_txBuffer.get(sample, control);
        sample *= 5;		// amplify by 12dB
        short signed_sample = (short)sample;
        out[i] = signed_sample;
    }
    ::pthread_mutex_unlock(&m_TXlock);

    return n;
}

