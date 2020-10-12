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

#include "gr_mod_dsss.h"


gr_mod_dsss_sptr make_gr_mod_dsss(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    return gnuradio::get_initial_sptr(new gr_mod_dsss(sps, samp_rate, carrier_freq,
                                                      filter_width));
}

gr_mod_dsss::gr_mod_dsss(int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_mod_dsss",
                      gr::io_signature::make (1, 1, sizeof (char)),
                      gr::io_signature::make (1, 1, sizeof (gr_complex)))
{
    std::vector<gr_complex> constellation;
    constellation.push_back(-1);
    constellation.push_back(1);
    std::vector<int> polys;
    polys.push_back(109);
    polys.push_back(79);

    static const int barker_13[] = {1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1};
    std::vector<int> dsss_code (barker_13, barker_13 + sizeof(barker_13) / sizeof(barker_13[0]) );

    _samples_per_symbol = sps; // 25
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    int if_samp_rate = 5200;


    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _unpacked_to_packed = gr::blocks::unpacked_to_packed_bb::make(1,gr::GR_MSB_FIRST);
    _scrambler = gr::digital::scrambler_bb::make(0x8A, 0x7F ,7);

    gr::fec::code::cc_encoder::sptr encoder = gr::fec::code::cc_encoder::make(80, 7, 2, polys);
    _encode_ccsds = gr::fec::encoder::make(encoder, 1, 1);

    _unpacked_to_packed = gr::blocks::unpacked_to_packed_bb::make(1, gr::GR_MSB_FIRST);
    _ccsds_encoder = gr::fec::encode_ccsds_27_bb::make();
    _unpacked_to_packed2 = gr::blocks::unpacked_to_packed_bb::make(1,gr::GR_MSB_FIRST);

    _chunks_to_symbols = gr::digital::chunks_to_symbols_bc::make(constellation);


    _resampler = gr::filter::rational_resampler_base_ccf::make(_samples_per_symbol, 1,
                                  gr::filter::firdes::root_raised_cosine(_samples_per_symbol,
                                        _samples_per_symbol,1,0.35,11 * _samples_per_symbol));
    _resampler->set_thread_priority(99);

    _dsss_encoder = gr::dsss::dsss_encoder_bb::make(dsss_code);
    _amplify = gr::blocks::multiply_const_cc::make(0.65,1);
    _bb_gain = gr::blocks::multiply_const_cc::make(1,1);
    _filter = gr::filter::fft_filter_ccf::make(
                1,gr::filter::firdes::low_pass_2(
                    1, if_samp_rate, _filter_width, 1200, 60, gr::filter::firdes::WIN_BLACKMAN_HARRIS));
    _resampler_if = gr::filter::rational_resampler_base_ccf::make(50,13,
                        gr::filter::firdes::low_pass(50.0,if_samp_rate*50,_filter_width,_filter_width*5));
    _resampler_rf = gr::filter::rational_resampler_base_ccf::make(50, 1,
                        gr::filter::firdes::low_pass(50,_samp_rate,_filter_width,_filter_width*5));


    connect(self(),0,_packed_to_unpacked,0);
    connect(_packed_to_unpacked,0,_scrambler,0);
    connect(_scrambler,0,_encode_ccsds,0);
    //connect(_scrambler,0,_unpacked_to_packed2,0);
    //connect(_unpacked_to_packed2,0,_ccsds_encoder,0);
    connect(_encode_ccsds,0,_unpacked_to_packed,0);
    connect(_unpacked_to_packed,0,_dsss_encoder,0);
    connect(_dsss_encoder,0,_chunks_to_symbols,0);
    connect(_chunks_to_symbols,0,_resampler,0);
    connect(_resampler,0,_amplify,0);
    connect(_amplify,0,_bb_gain,0);
    connect(_bb_gain,0,_resampler_if,0);
    connect(_resampler_if,0,_resampler_rf,0);
    connect(_resampler_rf,0,self(),0);

}

void gr_mod_dsss::set_bb_gain(float value)
{
    _bb_gain->set_k(value);
}




