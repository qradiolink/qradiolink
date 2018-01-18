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

#ifndef GR_DEMOD_4FSK_SDR_H
#define GR_DEMOD_4FSK_SDR_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/endianness.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/digital/clock_recovery_mm_ff.h>
#include <gnuradio/blocks/unpack_k_bits_bb.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/blocks/multiply_const_cc.h>
#include <gnuradio/blocks/complex_to_mag.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/digital/constellation.h>
#include <gnuradio/digital/constellation_decoder_cb.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/digital/descrambler_bb.h>
#include <gnuradio/blocks/complex_to_mag_squared.h>
#include "gr_4fsk_discriminator.h"

class gr_demod_4fsk_sdr;

typedef boost::shared_ptr<gr_demod_4fsk_sdr> gr_demod_4fsk_sdr_sptr;
gr_demod_4fsk_sdr_sptr make_gr_demod_4fsk_sdr(int sps=125, int samp_rate=250000, int carrier_freq=1700,
                                          int filter_width=8000);

class gr_demod_4fsk_sdr : public gr::hier_block2
{
public:
    explicit gr_demod_4fsk_sdr(std::vector<int> signature, int sps=4, int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=1800);


private:
    gr::blocks::unpack_k_bits_bb::sptr _unpack;

    gr::filter::fft_filter_ccc::sptr _filter1;
    gr::filter::fft_filter_ccc::sptr _filter2;
    gr::filter::fft_filter_ccc::sptr _filter3;
    gr::filter::fft_filter_ccc::sptr _filter4;
    gr::blocks::complex_to_mag::sptr _mag1;
    gr::blocks::complex_to_mag::sptr _mag2;
    gr::blocks::complex_to_mag::sptr _mag3;
    gr::blocks::complex_to_mag::sptr _mag4;
    gr_4fsk_discriminator_sptr _discriminator;

    gr::blocks::multiply_const_cc::sptr _multiply_symbols;
    gr::analog::quadrature_demod_cf::sptr _freq_demod;
    gr::blocks::float_to_complex::sptr _float_to_complex;
    gr::filter::fft_filter_fff::sptr _symbol_filter;
    gr::digital::clock_recovery_mm_ff::sptr _clock_recovery;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::digital::constellation_decoder_cb::sptr _constellation_receiver;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::digital::descrambler_bb::sptr _descrambler;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    int _target_samp_rate;

};

#endif // GR_DEMOD_QPSK_SDR_H
