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

#ifndef GR_MOD_MMDVM_MULTI2_H
#define GR_MOD_MMDVM_MULTI2_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/blocks/short_to_float.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/pfb_synthesizer_ccf.h>
#include <gnuradio/filter/rational_resampler.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/null_source.h>
#include "gr_mmdvm_source.h"
#include "gr_zero_idle_bursts.h"
#include "src/bursttimer.h"


class gr_mod_mmdvm_multi2;

typedef std::shared_ptr<gr_mod_mmdvm_multi2> gr_mod_mmdvm_multi2_sptr;
gr_mod_mmdvm_multi2_sptr make_gr_mod_mmdvm_multi2(BurstTimer *burst_timer, int num_channels=3,
                                                int channel_separation=25000, bool use_tdma=true,
                                                int sps=25, int samp_rate=MMDVM_SAMPLE_RATE, int carrier_freq=1700,
                                          int filter_width=5000);

class gr_mod_mmdvm_multi2 : public gr::hier_block2
{
public:
    explicit gr_mod_mmdvm_multi2(BurstTimer *burst_timer, int num_channels=3, int channel_separation=25000,
                                bool use_tdma=true,
                                int sps=25, int samp_rate=MMDVM_SAMPLE_RATE, int carrier_freq=1700,
                             int filter_width=5000);

    void set_bb_gain(float value);

private:

    gr::analog::frequency_modulator_fc::sptr _fm_modulator[MAX_MMDVM_CHANNELS];
    gr::filter::rational_resampler_ccf::sptr _resampler[MAX_MMDVM_CHANNELS];
    gr::blocks::multiply_const_cc::sptr _amplify[MAX_MMDVM_CHANNELS];
    gr::blocks::multiply_const_cc::sptr _bb_gain;
    gr::blocks::multiply_const_ff::sptr _audio_amplify[MAX_MMDVM_CHANNELS];
    gr::filter::fft_filter_ccf::sptr _filter[MAX_MMDVM_CHANNELS];
    gr::blocks::short_to_float::sptr _short_to_float[MAX_MMDVM_CHANNELS];
    gr::filter::pfb_synthesizer_ccf::sptr _synthesizer;
    gr_mmdvm_source_sptr _mmdvm_source;
    gr_zero_idle_bursts_sptr _zero_idle[MAX_MMDVM_CHANNELS];
    gr::blocks::multiply_const_cc::sptr _divide_level;
    gr::blocks::null_source::sptr _null_source[10];


    int _samp_rate;
    int _sps;
    int _carrier_freq;
    int _filter_width;
    int _num_channels;
    bool _use_tdma;

};
#endif // GR_MOD_MMDVM_MULTI2_H
