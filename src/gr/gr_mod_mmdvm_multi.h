// Written by Adrian Musceac YO8RZZ , started July 2021.
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

#ifndef GR_MOD_MMDVM_MULTI_H
#define GR_MOD_MMDVM_MULTI_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/blocks/short_to_float.h>
#include <gnuradio/blocks/add_blk.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/rational_resampler_base.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/rotator_cc.h>
#include "gr_mmdvm_source.h"
#include "gr_mmdvm_flow_pad.h"
#include "src/bursttimer.h"


class gr_mod_mmdvm_multi;

typedef boost::shared_ptr<gr_mod_mmdvm_multi> gr_mod_mmdvm_multi_sptr;
gr_mod_mmdvm_multi_sptr make_gr_mod_mmdvm_multi(BurstTimer *burst_timer, int sps=25, int samp_rate=1000000, int carrier_freq=1700,
                                          int filter_width=6250);

class gr_mod_mmdvm_multi : public gr::hier_block2
{
public:
    explicit gr_mod_mmdvm_multi(BurstTimer *burst_timer, int sps=25, int samp_rate=1000000, int carrier_freq=1700,
                             int filter_width=6250);
    void set_bb_gain(float value);

private:

    gr::analog::frequency_modulator_fc::sptr _fm_modulator1;
    gr::analog::frequency_modulator_fc::sptr _fm_modulator2;
    gr::filter::rational_resampler_base_ccf::sptr _resampler1;
    gr::filter::rational_resampler_base_ccf::sptr _resampler2;
    gr::filter::rational_resampler_base_ccf::sptr _final_resampler;
    gr::blocks::multiply_const_cc::sptr _amplify1;
    gr::blocks::multiply_const_cc::sptr _amplify2;
    gr::blocks::multiply_const_cc::sptr _bb_gain;
    gr::blocks::multiply_const_ff::sptr _audio_amplify1;
    gr::blocks::multiply_const_ff::sptr _audio_amplify2;
    gr::filter::fft_filter_ccf::sptr _filter1;
    gr::filter::fft_filter_ccf::sptr _filter2;
    gr::blocks::short_to_float::sptr _short_to_float1;
    gr::blocks::short_to_float::sptr _short_to_float2;
    gr::blocks::rotator_cc::sptr _rotator2;
    gr::blocks::add_cc::sptr _add;
    gr_mmdvm_source_sptr _mmdvm_source1;
    gr_mmdvm_source_sptr _mmdvm_source2;
    gr::blocks::multiply_const_cc::sptr _divide_level;
    gr_mmdvm_flow_pad_sptr _mmdvm_flow_pad;


    int _samp_rate;
    int _sps;
    int _carrier_freq;
    int _filter_width;

};
#endif // GR_MOD_MMDVM_MULTI_H
