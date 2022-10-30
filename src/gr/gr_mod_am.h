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

#ifndef GR_MOD_AM_H
#define GR_MOD_AM_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/rational_resampler.h>
#include <gnuradio/analog/agc2_ff.h>
#include <gnuradio/analog/feedforward_agc_cc.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/add_blk.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/analog/rail_ff.h>

class gr_mod_am;

typedef std::shared_ptr<gr_mod_am> gr_mod_am_sptr;
gr_mod_am_sptr make_gr_mod_am(int sps=125, int samp_rate=250000, int carrier_freq=1700,
                                          int filter_width=8000);

class gr_mod_am : public gr::hier_block2
{
public:
    explicit gr_mod_am(int sps=125, int samp_rate=250000, int carrier_freq=1700,
                             int filter_width=8000);
    void set_filter_width(int filter_width);
    void set_bb_gain(float value);

private:
    gr::filter::rational_resampler_ccf::sptr _resampler;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::blocks::multiply_const_cc::sptr _bb_gain;
    gr::blocks::multiply_const_ff::sptr _audio_amplify;
    gr::filter::fft_filter_fff::sptr _audio_filter;
    gr::filter::fft_filter_ccc::sptr _filter;
    gr::analog::agc2_ff::sptr _agc;
    gr::analog::feedforward_agc_cc::sptr _feed_forward_agc;
    gr::analog::sig_source_f::sptr _signal_source;
    gr::blocks::add_ff::sptr _add;
    gr::blocks::float_to_complex::sptr _float_to_complex;
    gr::analog::rail_ff::sptr _rail;


    int _samp_rate;
    int _sps;
    int _carrier_freq;
    int _filter_width;

};

#endif // GR_MOD_AM_H
