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

#include "gr_zero_idle_bursts.h"

static const pmt::pmt_t ZERO_TAG = pmt::string_to_symbol("zero_samples");

gr_zero_idle_bursts_sptr
make_gr_zero_idle_bursts (unsigned int delay)
{
    return gnuradio::get_initial_sptr(new gr_zero_idle_bursts(delay));
}

gr_zero_idle_bursts::gr_zero_idle_bursts(unsigned int delay) :
        gr::sync_block("gr_zero_idle_bursts",
                       gr::io_signature::make (1, 1, sizeof (gr_complex)),
                       gr::io_signature::make (1, 1, sizeof (gr_complex)))
{
    _sample_counter = 0;
    _delay = delay;
    if(delay > 0)
    {
        set_history(2 * SAMPLES_PER_SLOT);
    }
}

gr_zero_idle_bursts::~gr_zero_idle_bursts()
{

}

int gr_zero_idle_bursts::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{

    gr_complex *out = (gr_complex*)(output_items[0]);
    gr_complex *in = (gr_complex*)(input_items[0]);
    std::vector<gr::tag_t> tags;
    uint64_t nitems = nitems_written(0);


    get_tags_in_window(tags, 0, 0, noutput_items, ZERO_TAG);
    if (!tags.empty()) {

        std::sort(tags.begin(), tags.end(), gr::tag_t::offset_compare);
    }

    for(int i = 0;i < noutput_items; i++)
    {
        for (gr::tag_t& tag : tags)
        {
            if(tag.offset == nitems + (uint64_t)i + _delay)
            {
                _sample_counter = pmt::to_uint64(tag.value);
                break;
            }
        }
        if(_sample_counter > 0)
        {
            out[i] = 0 + 0j;
            _sample_counter--;
        }
        else
        {
            out[i] = in[i];
        }
    }

    return noutput_items;
}

