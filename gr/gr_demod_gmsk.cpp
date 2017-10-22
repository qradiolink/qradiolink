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

#include "gr_demod_gmsk.h"

gr_demod_gmsk::gr_demod_gmsk(QObject *parent, int sps, int samp_rate, int carrier_freq, int filter_width, float mod_index) :
    QObject(parent)
{
    _samples_per_symbol = sps;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("gmsk_demodulator");

    _audio_source = gr::audio::source::make(_samp_rate);
    _float_to_complex = gr::blocks::float_to_complex::make();
    _multiply = gr::blocks::multiply_cc::make();
    _signal_source = gr::analog::sig_source_c::make(_samp_rate,gr::analog::GR_COS_WAVE,-_carrier_freq,1);

    _band_pass_filter_1 = gr::filter::fir_filter_ccf::make(
                1,gr::filter::firdes::low_pass(
                    1, _samp_rate, _filter_width,600));
    _band_pass_filter_2 = gr::filter::fir_filter_ccf::make(
                1,gr::filter::firdes::low_pass(
                    1, _samp_rate, _filter_width,600));

    _quadrature_demodulator = gr::analog::quadrature_demod_cf::make(_samples_per_symbol / (M_PI/2*_modulation_index));
    _float_to_complex2 = gr::blocks::float_to_complex::make();
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(_samples_per_symbol, 0.25*0.175*0.175, 0.5, 0.175,
                                                              0.005);
    _costas_loop = gr::digital::costas_loop_cc::make(0.0628,2);
    _complex_to_real = gr::blocks::complex_to_real::make();
    _binary_slicer = gr::digital::binary_slicer_fb::make();
    _diff_decoder = gr::digital::diff_decoder_bb::make(2);
    _unpacked_to_packed = gr::blocks::unpacked_to_packed_bb::make(1,gr::GR_MSB_FIRST);
    _vector_sink = make_gr_vector_sink();






    _top_block->connect(_audio_source,0,_float_to_complex,0);

    _top_block->connect(_float_to_complex,0,_multiply,0);
    _top_block->connect(_signal_source,0,_multiply,1);
    _top_block->connect(_multiply,0,_band_pass_filter_1,0);
    _top_block->connect(_band_pass_filter_1,0,_quadrature_demodulator,0);
    //_top_block->connect(_band_pass_filter_2,0,_quadrature_demodulator,0);
    _top_block->connect(_quadrature_demodulator,0,_float_to_complex2,0);
    _top_block->connect(_float_to_complex2,0,_clock_recovery,0);
    _top_block->connect(_clock_recovery,0,_costas_loop,0);
    _top_block->connect(_costas_loop,0,_complex_to_real,0);
    _top_block->connect(_complex_to_real,0,_binary_slicer,0);
    _top_block->connect(_binary_slicer,0,_diff_decoder,0);
    _top_block->connect(_diff_decoder,0,_vector_sink,0);
    //_top_block->connect(_unpacked_to_packed,0,_vector_sink,0);
}

void gr_demod_gmsk::start()
{
    _top_block->start();
}

void gr_demod_gmsk::stop()
{
    _top_block->stop();
    _top_block->wait();
}

std::vector<unsigned char>* gr_demod_gmsk::getData()
{
    std::vector<unsigned char> *data = _vector_sink->get_data();
    return data;
}
