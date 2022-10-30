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

#ifndef GR_DEMOD_GMSK_H
#define GR_DEMOD_GMSK_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/endianness.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/digital/symbol_sync_ff.h>
#include <gnuradio/blocks/unpack_k_bits_bb.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/rational_resampler.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/blocks/divide.h>
#include <gnuradio/analog/rail_ff.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/digital/descrambler_bb.h>
#include <gnuradio/fec/decoder.h>
#include <gnuradio/fec/cc_decoder.h>
#include <gnuradio/blocks/complex_to_mag.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/add_const_ff.h>
#include <gnuradio/blocks/delay.h>
#include <gnuradio/blocks/float_to_uchar.h>
#include <gnuradio/analog/quadrature_demod_cf.h>


class gr_demod_gmsk;

typedef std::shared_ptr<gr_demod_gmsk> gr_demod_gmsk_sptr;
gr_demod_gmsk_sptr make_gr_demod_gmsk(int sps=125, int samp_rate=250000, int carrier_freq=1700,
                                          int filter_width=8000);

class gr_demod_gmsk : public gr::hier_block2
{
public:
    explicit gr_demod_gmsk(std::vector<int> signature, int sps=4, int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=1800);

private:
    gr::blocks::multiply_const_cc::sptr _multiply_symbols;
    gr::blocks::float_to_complex::sptr _float_to_complex;
    gr::filter::rational_resampler_ccf::sptr _resampler;
    gr::digital::symbol_sync_ff::sptr _symbol_sync;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::digital::binary_slicer_fb::sptr _binary_slicer;
    gr::digital::descrambler_bb::sptr _descrambler;
    gr::digital::descrambler_bb::sptr _descrambler2;
    gr::blocks::delay::sptr _delay;
    gr::blocks::multiply_const_ff::sptr _multiply_const_fec;
    gr::blocks::add_const_ff::sptr _add;
    gr::blocks::float_to_uchar::sptr _float_to_uchar;
    gr::blocks::add_const_ff::sptr _add_const_fec;
    gr::fec::decoder::sptr _cc_decoder;
    gr::fec::decoder::sptr _cc_decoder2;
    gr::analog::quadrature_demod_cf::sptr _freq_demod;
    gr::filter::fft_filter_ccf::sptr _shaping_filter;


    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    int _target_samp_rate;

};

#endif // GR_DEMOD_GMSK_H
