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

#ifndef GR_DEMOD_SSB_H
#define GR_DEMOD_SSB_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/agc2_cc.h>
#include <gnuradio/filter/rational_resampler.h>
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/analog/rail_ff.h>
#include "cessb/clipper_cc.h"
#include "cessb/stretcher_cc.h"


class gr_demod_ssb;

typedef std::shared_ptr<gr_demod_ssb> gr_demod_ssb_sptr;
gr_demod_ssb_sptr make_gr_demod_ssb(int sps=125, int samp_rate=250000, int carrier_freq=1700,
                                          int filter_width=8000, int sb=0);

class gr_demod_ssb : public gr::hier_block2
{
public:
    explicit gr_demod_ssb(std::vector<int> signature, int sps=4, int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=1800, int sb=0);

    void set_squelch(int value);
    void set_filter_width(int filter_width);
    void set_agc_attack(float value);
    void set_agc_decay(float value);
    void set_gain(float value);

private:
    gr::filter::rational_resampler_ccf::sptr _resampler;
    gr::analog::pwr_squelch_cc::sptr _squelch;
    gr::filter::fft_filter_ccc::sptr _filter_usb;
    gr::filter::fft_filter_ccc::sptr _filter_lsb;
    gr::filter::fft_filter_fff::sptr _audio_filter;
    gr::analog::agc2_cc::sptr _agc;
    gr::blocks::complex_to_real::sptr _complex_to_real;
    gr::blocks::multiply_const_cc::sptr _if_gain;
    gr::blocks::multiply_const_ff::sptr _level_control;
    gr::cessb::clipper_cc::sptr _clipper;
    gr::cessb::stretcher_cc::sptr _stretcher;

    int _samples_per_symbol;
    int _samp_rate;
    int _sps;
    int _carrier_freq;
    int _filter_width;
    int _target_samp_rate;

};

#endif // GR_DEMOD_SSB_H
