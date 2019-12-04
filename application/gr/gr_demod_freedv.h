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

#ifndef GR_DEMOD_FREEDV_H
#define GR_DEMOD_FREEDV_H


#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/agc2_ff.h>
#include <gnuradio/analog/feedforward_agc_cc.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/blocks/float_to_short.h>
#include <gnuradio/blocks/short_to_float.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/vocoder/freedv_rx_ss.h>
#include <gnuradio/vocoder/freedv_api.h>


class gr_demod_freedv;

typedef boost::shared_ptr<gr_demod_freedv> gr_demod_freedv_sptr;
gr_demod_freedv_sptr make_gr_demod_freedv(int sps=125, int samp_rate=8000, int carrier_freq=1700,
                                          int filter_width=2000, int low_cutoff=200, int mode=gr::vocoder::freedv_api::MODE_1600, int sb=0);

class gr_demod_freedv : public gr::hier_block2
{
public:
    explicit gr_demod_freedv(std::vector<int> signature, int sps=125, int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=2000, int low_cutoff=200, int mode=gr::vocoder::freedv_api::MODE_1600, int sb=0);

    void set_squelch(int value);

private:

    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::filter::fft_filter_ccc::sptr _filter;
    gr::analog::agc2_ff::sptr _agc;
    gr::analog::feedforward_agc_cc::sptr _feed_forward_agc;
    gr::blocks::complex_to_real::sptr _complex_to_real;
    gr::blocks::float_to_short::sptr _float_to_short;
    gr::blocks::short_to_float::sptr _short_to_float;
    gr::blocks::multiply_const_ff::sptr _audio_gain;
    gr::blocks::multiply_const_ff::sptr _freedv_gain;
    gr::filter::fft_filter_fff::sptr _audio_filter;
    gr::vocoder::freedv_rx_ss::sptr _freedv;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    int _target_samp_rate;
};

#endif // GR_DEMOD_FREEDV_H
