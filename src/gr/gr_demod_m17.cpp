// Written by Adrian Musceac YO8RZZ , started September 2022.
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

#include "gr_demod_m17.h"

gr_demod_m17_sptr make_gr_demod_m17(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (unsigned char));
    return gnuradio::get_initial_sptr(new gr_demod_m17(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_m17::gr_demod_m17(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_m17",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (3, 3, signature))
{
    (void) sps;
    _target_samp_rate = 24000;
    _samples_per_symbol = 5;

    _samp_rate = samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    std::vector<gr_complex> constellation_points;
    constellation_points.push_back(-1.5+0j);
    constellation_points.push_back(-0.5+0j);
    constellation_points.push_back(0.5+0j);
    constellation_points.push_back(1.5f+0j);
    int ntaps = 50 * _samples_per_symbol;

    std::vector<int> pre_diff;

    gr::digital::constellation_rect::sptr constellation_4fsk = gr::digital::constellation_rect::make(
                constellation_points, pre_diff, 2, 4, 1, 1.0, 1.0);

    std::vector<float> taps = gr::filter::firdes::low_pass(3, _samp_rate * 3, _target_samp_rate/2,
                                _target_samp_rate/2, gr::fft::window::WIN_BLACKMAN_HARRIS);

    _resampler = gr::filter::rational_resampler_ccf::make(3, 125, taps);

    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
            1, _target_samp_rate, _filter_width, _filter_width, gr::fft::window::WIN_BLACKMAN_HARRIS) );

    _fm_demod = gr::analog::quadrature_demod_cf::make(_samples_per_symbol/M_PI);
    std::vector<float> symbol_filter_taps = gr::filter::firdes::root_raised_cosine(1.5,_target_samp_rate,
                                                                                   _target_samp_rate/_samples_per_symbol,
                                                                                   0.5, ntaps);
    _symbol_filter = gr::filter::fft_filter_fff::make(1,symbol_filter_taps);
    float symbol_rate ((float)_target_samp_rate / (float)_samples_per_symbol);
    float sps_deviation = 500.0f / symbol_rate;
    _symbol_sync = gr::digital::symbol_sync_ff::make(gr::digital::TED_MOD_MUELLER_AND_MULLER, _samples_per_symbol,
                                                    2 * M_PI / (symbol_rate / 50), 1.0, 0.2869, sps_deviation, 1, constellation_4fsk);
    _phase_mod = gr::analog::phase_modulator_fc::make(M_PI / 2);
    _complex_to_float = gr::blocks::complex_to_float::make();
    _interleave = gr::blocks::interleave::make(4);
    _slicer = gr::digital::binary_slicer_fb::make();
    _packer = gr::blocks::pack_k_bits_bb::make(2);
    _unpacker = gr::blocks::unpack_k_bits_bb::make(2);
    std::vector<int> map;
    map.push_back(3);
    map.push_back(1);
    map.push_back(2);
    map.push_back(0);
    _symbol_map = gr::digital::map_bb::make(map);

    connect(self(),0,_resampler,0);

    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_fm_demod,0);
    connect(_fm_demod,0,_symbol_filter,0);
    connect(_symbol_filter,0,_symbol_sync,0);
    connect(_symbol_sync,0,_phase_mod,0);
    connect(_phase_mod,0,self(),1);
    connect(_phase_mod,0,_complex_to_float,0);
    connect(_complex_to_float,0,_interleave,0);
    connect(_complex_to_float,1,_interleave,1);
    connect(_interleave,0,_slicer,0);
    connect(_slicer,0,_packer,0);
    connect(_packer,0,_symbol_map,0);
    connect(_symbol_map,0,_unpacker,0);
    connect(_unpacker,0,self(),2);

}




