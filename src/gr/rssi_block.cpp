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

#include "rssi_block.h"


rssi_block_sptr make_rssi_block(int level)
{
    return gnuradio::get_initial_sptr(new rssi_block(level));
}

rssi_block::rssi_block(float level)
    : gr::hier_block2 ("rssi_block",
                          gr::io_signature::make (1, 1, sizeof (gr_complex)),
                          gr::io_signature::make (1, 1, sizeof (float)))
{

    _mag_squared = gr::blocks::complex_to_mag_squared::make();
    _single_pole_filter = gr::filter::single_pole_iir_filter_ff::make(0.04);
    _log10 = gr::blocks::nlog10_ff::make();
    _multiply_const_ff = gr::blocks::multiply_const_ff::make(10);
    _moving_average = gr::blocks::moving_average_ff::make(2000,1,2000);
    _add_const = gr::blocks::add_const_ff::make(level);

    connect(self(),0,_mag_squared,0);
    connect(_mag_squared,0,_moving_average,0);
    connect(_moving_average,0,_single_pole_filter,0);
    connect(_single_pole_filter,0,_log10,0);
    connect(_log10,0,_multiply_const_ff,0);
    connect(_multiply_const_ff,0,_add_const,0);
    connect(_add_const,0,self(),0);
}

void rssi_block::set_level(float level)
{
    _add_const->set_k(level);
}
