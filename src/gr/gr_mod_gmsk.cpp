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

#include "gr_mod_gmsk.h"

gr_mod_gmsk_sptr make_gr_mod_gmsk(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_gmsk(sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_gmsk::gr_mod_gmsk(int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_gmsk",
                      gr::io_signature::make (1, 1, sizeof (char)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{
    std::vector<float> constellation;
    constellation.push_back(-1);
    constellation.push_back(1);
    std::vector<int> polys;
    polys.push_back(109);
    polys.push_back(79);

    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);

    _samples_per_symbol = sps;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    int nfilts = 35;
    float amplif = 0.9f;

    int second_interp = 5;
    int if_samp_rate = 200000;
    if(_samples_per_symbol == 10)
    {
        nfilts = 80;
    }
    if(_samples_per_symbol == 50)
    {
        nfilts = 55;
    }
    if(_samples_per_symbol == 100)
    {
        nfilts = 35;
    }
    if((nfilts % 2) == 0)
        nfilts += 1;

    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _scrambler = gr::digital::scrambler_bb::make(0x8A, 0x7F ,7);
    _map = gr::digital::map_bb::make(map);

     gr::fec::code::cc_encoder::sptr encoder = gr::fec::code::cc_encoder::make(80, 7, 2, polys);
    _encode_ccsds = gr::fec::encoder::make(encoder, 1, 1);

    _chunks_to_symbols = gr::digital::chunks_to_symbols_bf::make(constellation);
    _freq_modulator = gr::analog::frequency_modulator_fc::make((M_PI/2)/(_samples_per_symbol));
    _resampler = gr::filter::rational_resampler_fff::make(_samples_per_symbol, 1,
                    gr::filter::firdes::gaussian(_samples_per_symbol,
                                _samples_per_symbol,0.5,nfilts));
    _amplify = gr::blocks::multiply_const_cc::make(amplif,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    _resampler2 = gr::filter::rational_resampler_ccf::make(second_interp, 1,
                gr::filter::firdes::low_pass(second_interp,_samp_rate,_filter_width,_filter_width));

    connect(self(),0,_packed_to_unpacked,0);
    connect(_packed_to_unpacked,0,_scrambler,0);
    connect(_scrambler,0,_encode_ccsds,0);
    connect(_encode_ccsds,0,_map,0);
    connect(_map,0,_chunks_to_symbols,0);
    connect(_chunks_to_symbols,0,_resampler,0);
    connect(_resampler,0,_freq_modulator,0);
    connect(_freq_modulator,0,_amplify,0);
    connect(_amplify,0,_bb_gain,0);
    connect(_bb_gain,0,_resampler2,0);
    connect(_resampler2,0,self(),0);
}

void gr_mod_gmsk::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}




