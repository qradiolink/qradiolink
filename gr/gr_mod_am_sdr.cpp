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

#include "gr_mod_am_sdr.h"

gr_mod_am_sdr::gr_mod_am_sdr(QObject *parent, int samp_rate, int carrier_freq, int filter_width,
                               float mod_index, float device_frequency, float rf_gain,
                               std::string device_args, std::string device_antenna, int freq_corr) :
    QObject(parent)
{

    _device_frequency = device_frequency;
    _samp_rate =samp_rate;
    float target_samp_rate = 8000.0;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("am modulator sdr");

    _audio_source = make_gr_audio_source();
    _signal_source = gr::analog::sig_source_f::make(target_samp_rate,gr::analog::GR_COS_WAVE, 0, 0.01);
    _multiply = gr::blocks::multiply_cc::make();
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::low_pass(
                    1, target_samp_rate, _filter_width, 600, gr::filter::firdes::WIN_HAMMING));
    _float_to_complex = gr::blocks::float_to_complex::make();
    std::vector<float> interp_taps = gr::filter::firdes::low_pass(1, target_samp_rate,
                                                        4000, 4000);
    float rerate = (float)_samp_rate/target_samp_rate;
    _resampler = gr::filter::pfb_arb_resampler_ccf::make(rerate, interp_taps, 16);
    _amplify = gr::blocks::multiply_const_cc::make(80,1);
    _filter = gr::filter::fft_filter_ccc::make(
                1,gr::filter::firdes::complex_band_pass_2(
                    1, _samp_rate, -_filter_width, _filter_width, 600, 120, gr::filter::firdes::WIN_BLACKMAN_HARRIS));


    _osmosdr_sink = osmosdr::sink::make(device_args);
    _osmosdr_sink->set_sample_rate(_samp_rate);
    _osmosdr_sink->set_antenna(device_antenna);
    _osmosdr_sink->set_center_freq(_device_frequency);
    _osmosdr_sink->set_bandwidth(_samp_rate*20);
    osmosdr::gain_range_t range = _osmosdr_sink->get_gain_range();
    if (!range.empty())
    {
        double gain =  range.start() + rf_gain*(range.stop()-range.start());
        _osmosdr_sink->set_gain(gain);
    }


    _top_block->connect(_audio_source,0,_audio_filter,0);
    _top_block->connect(_audio_filter,0,_float_to_complex,0);
    _top_block->connect(_signal_source,0,_float_to_complex,1);
    _top_block->connect(_float_to_complex,0,_resampler,0);
    _top_block->connect(_resampler,0,_amplify,0);
    _top_block->connect(_amplify,0,_filter,0);

    _top_block->connect(_filter,0,_osmosdr_sink,0);
}

gr_mod_am_sdr::~gr_mod_am_sdr()
{
    _osmosdr_sink.reset();
}

void gr_mod_am_sdr::start()
{
    _top_block->start();
}

void gr_mod_am_sdr::stop()
{
    _top_block->stop();
    _top_block->wait();
}

int gr_mod_am_sdr::setData(std::vector<float> *data)
{
    return _audio_source->set_data(data);

}

void gr_mod_am_sdr::tune(long center_freq)
{
    _device_frequency = center_freq;
    _osmosdr_sink->set_center_freq(_device_frequency);
}

void gr_mod_am_sdr::set_power(float dbm)
{
    osmosdr::gain_range_t range = _osmosdr_sink->get_gain_range();
    if (!range.empty())
    {
        double gain =  range.start() + dbm*(range.stop()-range.start());
        _osmosdr_sink->set_gain(gain);
    }
}
