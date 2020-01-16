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


#ifndef GR_MOD_GMSK_H
#define GR_MOD_GMSK_H

#include <QObject>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_source_b.h>
#include <gnuradio/blocks/packed_to_unpacked.h>
#include <gnuradio/audio/sink.h>
#include <gnuradio/endianness.h>
#include <gnuradio/digital/chunks_to_symbols.h>
#include <gnuradio/blocks/repeat.h>
#include <gnuradio/filter/interp_fir_filter_fff.h>
#include <gnuradio/filter/interp_fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/analog/sig_source_waveform.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/digital/diff_encoder_bb.h>
#include <vector>
#include "gr_vector_source.h"

class gr_mod_gmsk : public QObject
{
    Q_OBJECT
public:
    explicit gr_mod_gmsk(QObject *parent = 0, int sps=4, int samp_rate=8000, int carrier_freq=1600, int filter_width=1100, float mod_index=1);

signals:

public slots:
    void start();
    void stop();
    int setData(std::vector<u_int8_t> *data);

private:
    gr::top_block_sptr _top_block;
    //gr::blocks::vector_source_b::sptr _vector_source;
    gr_vector_source_sptr _vector_source;
    gr::blocks::packed_to_unpacked_bb::sptr _packed_to_unpacked;
    gr::digital::chunks_to_symbols_bf::sptr _chunks_to_symbols;
    gr::blocks::repeat::sptr _repeat;
    gr::filter::interp_fir_filter_fff::sptr _interp_gaussian_filter;
    gr::analog::frequency_modulator_fc::sptr _frequency_modulator;
    gr::blocks::multiply_cc::sptr _multiply;
    gr::analog::sig_source_c::sptr _signal_source;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::filter::fir_filter_ccf::sptr _band_pass_filter_1;
    gr::filter::fir_filter_ccf::sptr _band_pass_filter_2;
    gr::blocks::complex_to_real::sptr _complex_to_real;
    gr::audio::sink::sptr _audio_sink;
    gr::digital::diff_encoder_bb::sptr _diff_encoder;


    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _modulation_index;

};

#endif // GR_MOD_GMSK_H
