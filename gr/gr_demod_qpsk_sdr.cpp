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

#include "gr_demod_qpsk_sdr.h"

gr_demod_qpsk_sdr_sptr make_gr_demod_qpsk_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (char));
    return gnuradio::get_initial_sptr(new gr_demod_qpsk_sdr(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_qpsk_sdr::gr_demod_qpsk_sdr(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_qpsk_sdr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (3, 3, signature))
{

    int decimation;
    int interpolation;
    if(sps > 2)
    {
        interpolation = 1;
        decimation = 50;
        _samples_per_symbol = sps*2/25;
        _target_samp_rate = 20000;
    }
    else
    {
        interpolation = 1;
        decimation = 4;
        _samples_per_symbol = sps;
        _target_samp_rate = 250000;
    }
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    int filter_slope = 600;
    if(_target_samp_rate > 100000)
        filter_slope = 5000;

    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);
    map.push_back(3);
    map.push_back(2);

    gr::digital::constellation_dqpsk::sptr constellation = gr::digital::constellation_dqpsk::make();

    unsigned int flt_size = 32;
    /*
    gr::digital::constellation_expl_rect::sptr constellation = gr::digital::constellation_expl_rect::make(
                constellation->points(),pre_diff_code,4,2,2,1,1,const_map);
    */

    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _filter_width, 12000);

    _resampler = gr::filter::rational_resampler_base_ccf::make(interpolation, decimation, taps);

    _agc = gr::analog::agc2_cc::make(1, 1, 1, 0);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                                1, _target_samp_rate, _filter_width, filter_slope,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    float gain_mu, omega_rel_limit;
    if(sps > 2)
    {
        gain_mu = 0.005;
        omega_rel_limit = 0.0005;
    }
    else
    {
        gain_mu = 0.001;
        omega_rel_limit = 0.0001;
    }

    _shaping_filter = gr::filter::fft_filter_ccf::make(
                1, gr::filter::firdes::root_raised_cosine(1,_target_samp_rate,_target_samp_rate/_samples_per_symbol,0.35,32));
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(_samples_per_symbol, 0.025*gain_mu*gain_mu, 0.5, gain_mu,
                                                              omega_rel_limit);
    std::vector<float> pfb_taps = gr::filter::firdes::root_raised_cosine(flt_size,flt_size, 1, 0.35, flt_size * 11 * _samples_per_symbol);
    _clock_sync = gr::digital::pfb_clock_sync_ccf::make(_samples_per_symbol,0.0628,pfb_taps);
    _costas_loop = gr::digital::costas_loop_cc::make(2*M_PI/100,4);
    _equalizer = gr::digital::cma_equalizer_cc::make(11,1,0.0001,1);
    _fll = gr::digital::fll_band_edge_cc::make(sps, 0.35, 32, 2*M_PI/100);
    _diff_decoder = gr::digital::diff_decoder_bb::make(4);
    _map = gr::digital::map_bb::make(map);
    _unpack = gr::blocks::unpack_k_bits_bb::make(2);
    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _constellation_receiver = gr::digital::constellation_decoder_cb::make(constellation);


    connect(self(),0,_resampler,0);
    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    if(sps > 2)
    {
        connect(_filter,0,_fll,0);
        //connect(_shaping_filter,0,_agc,0);
        connect(_fll,0,_agc,0);
    }
    else
    {
        connect(_filter,0,_agc,0);
    }

    connect(_agc,0,_clock_recovery,0);
    connect(_clock_recovery,0,_equalizer,0);
    connect(_equalizer,0,_costas_loop,0);
    connect(_costas_loop,0,self(),1);
    connect(_costas_loop,0,_constellation_receiver,0);
    connect(_constellation_receiver,0,_diff_decoder,0);
    connect(_diff_decoder,0,_map,0);
    connect(_map,0,_unpack,0);
    connect(_unpack,0,_descrambler,0);
    connect(_descrambler,0,self(),2);

}
