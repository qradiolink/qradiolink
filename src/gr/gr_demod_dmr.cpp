// Written by Adrian Musceac YO8RZZ , started July 2021.
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

#include "gr_demod_dmr.h"
#include "src/DMR/constants.h"

gr_demod_dmr_sptr make_gr_demod_dmr(int sps, int samp_rate)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (unsigned char));
    return gnuradio::get_initial_sptr(new gr_demod_dmr(signature, sps, samp_rate));
}



gr_demod_dmr::gr_demod_dmr(std::vector<int>signature, int sps, int samp_rate) :
    gr::hier_block2 ("gr_demod_dmr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (3, 3, signature))
{
    _sps = sps;
    _target_samp_rate = 24000;
    _samp_rate = samp_rate;
    unsigned int samples_per_symbol = 5;
    std::vector<gr_complex> constellation_points;
    constellation_points.push_back(-1.5+0j);
    constellation_points.push_back(-0.5+0j);
    constellation_points.push_back(0.5+0j);
    constellation_points.push_back(1.5+0j);
    int ntaps = 25 * samples_per_symbol;

    std::vector<int> pre_diff;

    gr::digital::constellation_rect::sptr constellation_4fsk = gr::digital::constellation_rect::make(
                constellation_points, pre_diff, 2, 4, 1, 1.0, 1.0);

    _filter_width = 5000.0f;

    std::vector<float> taps = gr::filter::firdes::low_pass_2(3, _samp_rate * 3, _filter_width,
                                2000, 60, gr::fft::window::WIN_BLACKMAN_HARRIS);
    unsigned int resampler_delay = (taps.size() - 1) / (2);
    _resampler = gr::filter::rational_resampler_ccf::make(3, 125, taps);
    //_resampler->declare_sample_delay(resampler_delay * 3);
    _rssi_tag_block = make_rssi_tag_block();

    _phase_mod = gr::analog::phase_modulator_fc::make(M_PI / 2);
    std::vector<float> symbol_filter_taps = gr::filter::firdes::root_raised_cosine(1,_target_samp_rate,
                                _target_samp_rate/samples_per_symbol,0.2,ntaps);
    unsigned int filter_delay = (symbol_filter_taps.size() - 1) / 2;
    _symbol_filter = gr::filter::fft_filter_fff::make(
                1, symbol_filter_taps);
    //_symbol_filter->declare_sample_delay(filter_delay);
    float symbol_rate ((float)_target_samp_rate / (float)samples_per_symbol);
    float sps_deviation = 0.06;
    _symbol_sync = gr::digital::symbol_sync_ff::make(gr::digital::TED_MUELLER_AND_MULLER, samples_per_symbol,
                                                    2 * M_PI / 100.0f, 1.0, 0.2869, sps_deviation, 1, constellation_4fsk);
    _fm_demod = gr::analog::quadrature_demod_cf::make(_target_samp_rate/(M_PI/2*symbol_rate));
    _level_control = gr::blocks::multiply_const_ff::make(0.9/*3.66*/);
    _complex_to_float = gr::blocks::complex_to_float::make();
    _complex_to_float_corr = gr::blocks::complex_to_float::make();
    _float_to_complex_corr = gr::blocks::float_to_complex::make();
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

    std::vector<gr_complex> bs_data_symbols(BS_DATA_SYNC, BS_DATA_SYNC + sizeof(BS_DATA_SYNC) / sizeof(BS_DATA_SYNC[0]));
    std::vector<gr_complex> bs_voice_symbols(BS_VOICE_SYNC, BS_VOICE_SYNC + sizeof(BS_VOICE_SYNC) / sizeof(BS_VOICE_SYNC[0]));
    std::vector<gr_complex> ms_data_symbols(MS_DATA_SYNC, MS_DATA_SYNC + sizeof(MS_DATA_SYNC) / sizeof(MS_DATA_SYNC[0]));;
    std::vector<gr_complex> ms_voice_symbols(MS_VOICE_SYNC, MS_VOICE_SYNC + sizeof(MS_VOICE_SYNC) / sizeof(MS_VOICE_SYNC[0]));

    _corr_est_bs_data = gr::digital::corr_est_cc::make(bs_data_symbols, 5, 0, 0.99, gr::digital::THRESHOLD_DYNAMIC);
    _corr_est_bs_voice = gr::digital::corr_est_cc::make(bs_voice_symbols, 5, 0, 0.99, gr::digital::THRESHOLD_DYNAMIC);
    _corr_est_ms_data = gr::digital::corr_est_cc::make(ms_data_symbols, 5, 0, 0.9999, gr::digital::THRESHOLD_DYNAMIC);
    _corr_est_ms_voice = gr::digital::corr_est_cc::make(ms_voice_symbols, 5, 0, 0.9999, gr::digital::THRESHOLD_DYNAMIC);


    connect(self(),0,_resampler,0);
    connect(_resampler,0,self(),0);
    connect(_resampler,0,_rssi_tag_block,0);
    connect(_rssi_tag_block,0,_fm_demod,0);
    connect(_fm_demod,0,_symbol_filter,0);
    connect(_symbol_filter,0,_symbol_sync,0);
    //connect(_symbol_filter,0,_float_to_complex_corr,0);
    //connect(_level_control,0,_symbol_sync,0);
    //connect(_float_to_complex_corr,0,_corr_est_bs_data,0);
    //connect(_corr_est_bs_data,0,_corr_est_bs_voice,0);
    //connect(_corr_est_bs_data,0,_corr_est_ms_data,0);
    //connect(_corr_est_ms_data,0,_corr_est_ms_voice,0);
    //connect(_corr_est_bs_voice,0,_corr_est_ms_data,0);
    //connect(_corr_est_ms_data,0,_corr_est_ms_voice,0);
    //connect(_corr_est_bs_data,0,_complex_to_float_corr,0);
    //connect(_complex_to_float_corr,0,_corr_slicer,0);
    //connect(_corr_slicer,0,_level_control,0);
    connect(_symbol_sync,0,_level_control,0);
    connect(_level_control,0,_phase_mod,0);
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

void gr_demod_dmr::calibrate_rssi(float level)
{
    _rssi_tag_block->calibrate_rssi(level);
}


