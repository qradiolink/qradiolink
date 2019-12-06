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

#include "gr_mod_bpsk.h"

gr_mod_bpsk::gr_mod_bpsk(QObject *parent, int sps, int samp_rate, int carrier_freq, int filter_width, float mod_index) :
    QObject(parent)
{
    std::vector<gr_complex> constellation;
    constellation.push_back(-1);
    constellation.push_back(1);
    _samples_per_symbol = sps;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("bpsk_modulator");
    _vector_source = make_gr_vector_source();
    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _diff_encoder = gr::digital::diff_encoder_bb::make(2);
    _chunks_to_symbols = gr::digital::chunks_to_symbols_bc::make(constellation);
    _repeat = gr::blocks::repeat::make(8,_samples_per_symbol);
    _multiply = gr::blocks::multiply_cc::make(1);
    _signal_source = gr::analog::sig_source_c::make(_samp_rate,gr::analog::GR_COS_WAVE,_carrier_freq,1);
    _amplify = gr::blocks::multiply_const_cc::make(0.3,1);
    _band_pass_filter_1 = gr::filter::fir_filter_ccf::make(
                1,gr::filter::firdes::band_pass(
                    1, _samp_rate, _carrier_freq - _filter_width,_carrier_freq + _filter_width,600,gr::filter::firdes::WIN_HAMMING));
    _band_pass_filter_2 = gr::filter::fir_filter_ccf::make(
                1,gr::filter::firdes::band_pass(
                    1, _samp_rate, _carrier_freq - _filter_width,_carrier_freq + _filter_width,600,gr::filter::firdes::WIN_HAMMING));
    _complex_to_real = gr::blocks::complex_to_real::make();
    _audio_sink = gr::audio::sink::make(_samp_rate);

    _top_block->connect(_vector_source,0,_packed_to_unpacked,0);
    _top_block->connect(_packed_to_unpacked,0,_diff_encoder,0);
    _top_block->connect(_diff_encoder,0,_chunks_to_symbols,0);
    _top_block->connect(_chunks_to_symbols,0,_repeat,0);
    _top_block->connect(_repeat,0,_multiply,0);
    _top_block->connect(_signal_source,0,_multiply,1);
    _top_block->connect(_multiply,0,_amplify,0);
    _top_block->connect(_amplify,0,_band_pass_filter_1,0);
    //_top_block->connect(_band_pass_filter_1,0,_band_pass_filter_2,0);
    _top_block->connect(_band_pass_filter_1,0,_complex_to_real,0);
    _top_block->connect(_complex_to_real,0,_audio_sink,0);
}

void gr_mod_bpsk::start()
{
    _top_block->start();
}

void gr_mod_bpsk::stop()
{
    _top_block->stop();
    _top_block->wait();
}

int gr_mod_bpsk::setData(std::vector<u_int8_t> *data)
{
    return _vector_source->set_data(data);

}
