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

#include "gr_demod_4fsk_sdr.h"

gr_demod_4fsk_sdr_sptr make_gr_demod_4fsk_sdr(int sps, int samp_rate, int carrier_freq,
                                          int filter_width)
{
    std::vector<int> signature;
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (gr_complex));
    signature.push_back(sizeof (char));
    return gnuradio::get_initial_sptr(new gr_demod_4fsk_sdr(signature, sps, samp_rate, carrier_freq,
                                                      filter_width));
}



gr_demod_4fsk_sdr::gr_demod_4fsk_sdr(std::vector<int>signature, int sps, int samp_rate, int carrier_freq,
                                 int filter_width) :
    gr::hier_block2 ("gr_demod_4fsk_sdr",
                      gr::io_signature::make (1, 1, sizeof (gr_complex)),
                      gr::io_signature::makev (3, 3, signature))
{

    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;

    int rs,bw,decimation;
    float gain_mu, omega_rel_limit;
    if(sps == 50)
    {
        _target_samp_rate = 40000;
        _samples_per_symbol = sps*4/25;
        decimation = 25;
        rs = 5000;
        bw = 20000;
        gain_mu = 0.005;
        omega_rel_limit = 0.0001;
    }
    if(sps == 250)
    {
        _target_samp_rate = 20000;
        _samples_per_symbol = sps*2/25;
        decimation = 50;
        rs = 1000;
        bw = 2000;
        gain_mu = 0.025;
        omega_rel_limit = 0.005;
    }

    std::vector<unsigned int> const_map;
    const_map.push_back(0);
    const_map.push_back(1);
    const_map.push_back(2);
    const_map.push_back(3);

    std::vector<int> pre_diff_code;

    std::vector<gr_complex> constellation_points;
    constellation_points.push_back(-1.5+0j);
    constellation_points.push_back(-0.5+0j);
    constellation_points.push_back(0.5+0j);
    constellation_points.push_back(1.5+0j);

    gr::digital::constellation_expl_rect::sptr constellation = gr::digital::constellation_expl_rect::make(
                constellation_points,pre_diff_code,2,4,1,1,1,const_map);

    std::vector<float> taps = gr::filter::firdes::low_pass(1, _samp_rate, _filter_width, 12000);
    std::vector<float> symbol_filter_taps = gr::filter::firdes::low_pass(1.0,
                                 _target_samp_rate, _target_samp_rate/_samples_per_symbol, _target_samp_rate/_samples_per_symbol/20);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, decimation, taps);

    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                                1, _target_samp_rate, _filter_width,2000,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    //_freq_demod = gr::analog::quadrature_demod_cf::make(sps/(4*M_PI/2));

    _filter1 = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, -_filter_width,-_filter_width+rs,bw,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    _filter2 = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, -_filter_width+rs,0,bw,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    _filter3 = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, 0,_filter_width-rs,bw,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    _filter4 = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, _filter_width-rs,_filter_width, bw,gr::filter::firdes::WIN_BLACKMAN_HARRIS) );
    _mag1 = gr::blocks::complex_to_mag::make();
    _mag2 = gr::blocks::complex_to_mag::make();
    _mag3 = gr::blocks::complex_to_mag::make();
    _mag4 = gr::blocks::complex_to_mag::make();
    _discriminator = make_gr_4fsk_discriminator();


    _symbol_filter = gr::filter::fft_filter_fff::make(1,symbol_filter_taps);
    _clock_recovery = gr::digital::clock_recovery_mm_ff::make(_samples_per_symbol, 0.025*gain_mu*gain_mu, 0.5, gain_mu,
                                                              omega_rel_limit);
    _float_to_complex = gr::blocks::float_to_complex::make();
    _multiply_symbols = gr::blocks::multiply_const_cc::make(0.5);
    _unpack = gr::blocks::unpack_k_bits_bb::make(2);
    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _constellation_receiver = gr::digital::constellation_decoder_cb::make(constellation);


    connect(self(),0,_resampler,0);
    connect(_resampler,0,_filter,0);
    connect(_filter,0,self(),0);
    connect(_filter,0,_filter1,0);
    connect(_filter,0,_filter2,0);
    connect(_filter,0,_filter3,0);
    connect(_filter,0,_filter4,0);
    connect(_filter1,0,_mag1,0);
    connect(_filter2,0,_mag2,0);
    connect(_filter3,0,_mag3,0);
    connect(_filter4,0,_mag4,0);
    connect(_mag1,0,_discriminator,0);
    connect(_mag2,0,_discriminator,1);
    connect(_mag3,0,_discriminator,2);
    connect(_mag4,0,_discriminator,3);
    connect(_discriminator,0,_symbol_filter,0);

    //connect(_filter,0,_freq_demod,0);
    //connect(_freq_demod,0,_float_to_complex,0);
    connect(_symbol_filter,0,_clock_recovery,0);
    connect(_clock_recovery,0,_float_to_complex,0);
    connect(_float_to_complex,0,_multiply_symbols,0);
    connect(_multiply_symbols,0,self(),1);

    connect(_multiply_symbols,0,_constellation_receiver,0);
    connect(_constellation_receiver,0,_unpack,0);
    connect(_unpack,0,_descrambler,0);
    connect(_descrambler,0,self(),2);

}

