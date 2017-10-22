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

gr_mod_gmsk::gr_mod_gmsk(QObject *parent, int sps, int samp_rate, int carrier_freq, int filter_width, float mod_index) :
    QObject(parent)
{
    std::vector<float> constellation;
    constellation.push_back(-1);
    constellation.push_back(1);
    _samples_per_symbol = sps;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("gmsk_modulator");
    _vector_source = make_gr_vector_source();
    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _diff_encoder = gr::digital::diff_encoder_bb::make(2);
    _chunks_to_symbols = gr::digital::chunks_to_symbols_bf::make(constellation,1);
    _repeat = gr::blocks::repeat::make(4,_samples_per_symbol);
    _interp_gaussian_filter = gr::filter::interp_fir_filter_fff::make(
                1,gr::filter::firdes::gaussian(1, _samples_per_symbol, 0.25, 2));
    _frequency_modulator = gr::analog::frequency_modulator_fc::make((M_PI/2*_modulation_index) / _samples_per_symbol);
    _multiply = gr::blocks::multiply_cc::make(1);
    _signal_source = gr::analog::sig_source_c::make(_samp_rate,gr::analog::GR_COS_WAVE,_carrier_freq,1);
    _amplify = gr::blocks::multiply_const_cc::make(0.5,1);
    _band_pass_filter_1 = gr::filter::fir_filter_ccf::make(
                1,gr::filter::firdes::band_pass(
                    1, _samp_rate, _carrier_freq - _filter_width,_carrier_freq + _filter_width,600));
    _band_pass_filter_2 = gr::filter::fir_filter_ccf::make(
                1,gr::filter::firdes::band_pass(
                    1, _samp_rate, _carrier_freq - _filter_width,_carrier_freq + _filter_width,600));
    _complex_to_real = gr::blocks::complex_to_real::make();
    _audio_sink = gr::audio::sink::make(_samp_rate);

    _top_block->connect(_vector_source,0,_packed_to_unpacked,0);
    _top_block->connect(_packed_to_unpacked,0,_diff_encoder,0);
    _top_block->connect(_diff_encoder,0,_chunks_to_symbols,0);
    _top_block->connect(_chunks_to_symbols,0,_repeat,0);
    _top_block->connect(_repeat,0,_interp_gaussian_filter,0);
    _top_block->connect(_interp_gaussian_filter,0,_frequency_modulator,0);
    _top_block->connect(_frequency_modulator,0,_multiply,0);
    _top_block->connect(_signal_source,0,_multiply,1);
    _top_block->connect(_multiply,0,_amplify,0);
    _top_block->connect(_amplify,0,_band_pass_filter_1,0);
    //_top_block->connect(_band_pass_filter_1,0,_band_pass_filter_2,0);
    _top_block->connect(_band_pass_filter_1,0,_complex_to_real,0);
    _top_block->connect(_complex_to_real,0,_audio_sink,0);

}

void gr_mod_gmsk::start()
{
    _top_block->start();
}

void gr_mod_gmsk::stop()
{
    _top_block->stop();
    _top_block->wait();
}

int gr_mod_gmsk::setData(std::vector<u_int8_t> *data)
{
    return _vector_source->set_data(data);

}
