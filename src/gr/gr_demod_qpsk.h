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

#ifndef GR_DEMOD_QPSK_H
#define GR_DEMOD_QPSK_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/endianness.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/blocks/complex_to_float.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/digital/diff_phasor_cc.h>
#include <gnuradio/blocks/interleave.h>
#include <gnuradio/fec/decoder.h>
#include <gnuradio/fec/cc_decoder.h>
#include <gnuradio/digital/costas_loop_cc.h>
#include <gnuradio/digital/cma_equalizer_cc.h>
#include <gnuradio/digital/symbol_sync_cc.h>
#include <gnuradio/analog/agc2_cc.h>
#include <gnuradio/digital/fll_band_edge_cc.h>
#include <gnuradio/filter/rational_resampler_base.h>
#include <gnuradio/digital/constellation.h>
#include <gnuradio/digital/constellation_decoder_cb.h>
#include <gnuradio/digital/pfb_clock_sync_ccf.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/digital/descrambler_bb.h>
#include <gnuradio/blocks/float_to_uchar.h>
#include <gnuradio/blocks/add_const_ff.h>


class gr_demod_qpsk;

typedef boost::shared_ptr<gr_demod_qpsk> gr_demod_qpsk_sptr;
gr_demod_qpsk_sptr make_gr_demod_qpsk(int sps=125, int samp_rate=250000, int carrier_freq=1700,
                                          int filter_width=8000);

class gr_demod_qpsk : public gr::hier_block2
{
public:
    explicit gr_demod_qpsk(std::vector<int> signature, int sps=4, int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=1800);


private:
    gr::digital::cma_equalizer_cc::sptr _equalizer;
    gr::analog::agc2_cc::sptr _agc;
    gr::digital::fll_band_edge_cc::sptr _fll;
    gr::digital::pfb_clock_sync_ccf::sptr _clock_sync;
    gr::digital::symbol_sync_cc::sptr _symbol_sync;
    gr::digital::costas_loop_cc::sptr _costas_loop;
    gr::digital::costas_loop_cc::sptr _costas_pll;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::filter::fft_filter_ccf::sptr _shaping_filter;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::digital::descrambler_bb::sptr _descrambler;
    gr::fec::decoder::sptr _decode_ccsds;
    gr::digital::diff_phasor_cc::sptr _diff_phasor;
    gr::blocks::multiply_const_cc::sptr _rotate_const;
    gr::blocks::multiply_const_ff::sptr _multiply_const_fec;
    gr::blocks::complex_to_float::sptr _complex_to_float;
    gr::blocks::interleave::sptr _interleave;
    gr::blocks::float_to_uchar::sptr _float_to_uchar;
    gr::blocks::add_const_ff::sptr _add_const_fec;


    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    int _target_samp_rate;

};

#endif // GR_DEMOD_QPSK_H
