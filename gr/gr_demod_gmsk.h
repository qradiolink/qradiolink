// Written by Adrian Musceac YO8RZZ at gmail dot com, started March 2016.
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

#ifndef GR_DEMOD_GMSK_H
#define GR_DEMOD_GMSK_H

#include <QObject>
#include <gnuradio/audio/source.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/top_block.h>
#include <gnuradio/endianness.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/sig_source_waveform.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/digital/clock_recovery_mm_cc.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/blocks/unpacked_to_packed_bb.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/digital/costas_loop_cc.h>
#include <gnuradio/digital/diff_decoder_bb.h>
#include <vector>
#include "gr_vector_sink.h"

class gr_demod_gmsk : public QObject
{
    Q_OBJECT
public:
    explicit gr_demod_gmsk(QObject *parent = 0, int sps=4, int samp_rate=8000, int carrier_freq=1600, int filter_width=1100, float mod_index=1);

signals:

public slots:
    void start();
    void stop();
    std::vector<unsigned char> *getData();

private:
    gr::top_block_sptr _top_block;
    gr_vector_sink_sptr _vector_sink;
    gr::blocks::unpacked_to_packed_bb::sptr _unpacked_to_packed;
    gr::analog::quadrature_demod_cf::sptr _quadrature_demodulator;
    gr::blocks::multiply_cc::sptr _multiply;
    gr::analog::sig_source_c::sptr _signal_source;
    gr::filter::fir_filter_ccf::sptr _band_pass_filter_1;
    gr::filter::fir_filter_ccf::sptr _band_pass_filter_2;
    gr::blocks::complex_to_real::sptr _complex_to_real;
    gr::blocks::float_to_complex::sptr _float_to_complex;
    gr::blocks::float_to_complex::sptr _float_to_complex2;
    gr::audio::source::sptr _audio_source;
    gr::digital::clock_recovery_mm_cc::sptr _clock_recovery;
    gr::digital::binary_slicer_fb::sptr _binary_slicer;
    gr::digital::costas_loop_cc::sptr _costas_loop;
    gr::digital::diff_decoder_bb::sptr _diff_decoder;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _modulation_index;

};

#endif // GR_DEMOD_GMSK_H
