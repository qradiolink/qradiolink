// Written by Adrian Musceac YO8RZZ , started Nov 2023.
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

#include "rssi_tag_block.h"
#include <math.h>
#include <gnuradio/io_signature.h>

static const pmt::pmt_t RSSI_TAG = pmt::string_to_symbol("RSSI");

rssi_tag_block_sptr make_rssi_tag_block ()
{
    return gnuradio::get_initial_sptr(new rssi_tag_block());
}

rssi_tag_block::rssi_tag_block()
    : gr::sync_block ("rx_meter_c",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
    _calibration_level = 0.0f;
    _nitems = 0;
    _sum = 0.0f;
}

rssi_tag_block::~rssi_tag_block()
{
}

int rssi_tag_block::work (int noutput_items,
                      gr_vector_const_void_star &input_items,
                      gr_vector_void_star &output_items)
{
    const gr_complex *in = (const gr_complex*) input_items[0];
    gr_complex *out = (gr_complex*)(output_items[0]);
    float pwr = 0.0;

    for(int i = 0;i < noutput_items;i++)
    {
        pwr = in[i].real()*in[i].real() + in[i].imag()*in[i].imag();
        _sum += pwr*pwr;
        out[i] = in[i];
    }
    _nitems += noutput_items;
    if(_nitems > 2000)
    {
        float level = sqrt(_sum / (float)(_nitems));
        float db = (float) 10.0f * log10f(level + 1.0e-20) + _calibration_level;
        this->add_rssi_tag(db);
        _sum = 0;
        _nitems = 0;
    }
    return noutput_items;
}

// Add RSSI db tag to stream
void rssi_tag_block::add_rssi_tag(float db)
{
    const pmt::pmt_t t_val = pmt::from_float(db);
    this->add_item_tag(0, nitems_written(0), RSSI_TAG, t_val);
}

void rssi_tag_block::calibrate_rssi(float level)
{
    _calibration_level = level;
}
