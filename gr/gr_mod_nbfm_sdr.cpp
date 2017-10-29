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

#include "gr_mod_nbfm_sdr.h"

gr_mod_nbfm_sdr::gr_mod_nbfm_sdr(QObject *parent, int samp_rate, int carrier_freq,
                                 int filter_width, float mod_index, float device_frequency, float rf_gain,
                                 std::string device_args, std::string device_antenna, int freq_corr) :
    QObject(parent)
{
    _device_frequency = device_frequency;
    _samp_rate =samp_rate;
    float target_samp_rate = 8000;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("nbfm modulator sdr");

    _fm_modulator = gr::analog::frequency_modulator_fc::make(4*M_PI*_filter_width/target_samp_rate);
    _audio_source = gr::audio::source::make(target_samp_rate,"",true);
    _signal_source = gr::analog::sig_source_f::make(target_samp_rate,gr::analog::GR_COS_WAVE, 0, 1);
    _multiply = gr::blocks::multiply_ff::make();
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::band_pass(
                    1, target_samp_rate, 300, _filter_width, 600, gr::filter::firdes::WIN_HAMMING));

    static const float coeff[] =  {-0.026316914707422256, -0.2512197494506836, 1.5501943826675415,
                                   -0.2512197494506836, -0.026316914707422256};
    std::vector<float> iir_taps(coeff, coeff + sizeof(coeff) / sizeof(coeff[0]) );
    _emphasis_filter = gr::filter::fft_filter_ccf::make(1,iir_taps);

    std::vector<float> interp_taps = gr::filter::firdes::low_pass(1, target_samp_rate,
                                                        _filter_width, 2000);
    float rerate = (float)_samp_rate/target_samp_rate;
    _resampler = gr::filter::pfb_arb_resampler_ccf::make(rerate, interp_taps, 32);
    _amplify = gr::blocks::multiply_const_cc::make(10,1);
    _filter = gr::filter::fft_filter_ccf::make(
                1,gr::filter::firdes::low_pass(
                    1, _samp_rate, _filter_width, 600, gr::filter::firdes::WIN_HAMMING));

    _osmosdr_sink = osmosdr::sink::make(device_args);
    _osmosdr_sink->set_sample_rate(_samp_rate);
    _osmosdr_sink->set_antenna(device_antenna);
    _osmosdr_sink->set_center_freq(_device_frequency);
    _osmosdr_sink->set_gain(rf_gain);



    _top_block->connect(_audio_source,0,_audio_filter,0);
    //_top_block->connect(_signal_source,0,_multiply,1);
    _top_block->connect(_audio_filter,0,_fm_modulator,0);
    _top_block->connect(_fm_modulator,0,_emphasis_filter,0);
    _top_block->connect(_emphasis_filter,0,_resampler,0);
    _top_block->connect(_resampler,0,_amplify,0);
    _top_block->connect(_amplify,0,_filter,0);

    _top_block->connect(_filter,0,_osmosdr_sink,0);
}

void gr_mod_nbfm_sdr::start()
{
    _top_block->start();
}

void gr_mod_nbfm_sdr::stop()
{
    _top_block->stop();
    _top_block->wait();
}

void gr_mod_nbfm_sdr::tune(long center_freq)
{
    _device_frequency = center_freq;
    _osmosdr_sink->set_center_freq(_device_frequency);
}

void gr_mod_nbfm_sdr::set_power(int dbm)
{
    _osmosdr_sink->set_gain(dbm);
}
