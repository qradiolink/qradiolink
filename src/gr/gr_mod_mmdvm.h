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

#ifndef GR_MOD_MMDVM_H
#define GR_MOD_MMDVM_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/blocks/short_to_float.h>
#include <gnuradio/blocks/add_blk.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/rational_resampler.h>
#include <gnuradio/blocks/multiply_const.h>
#include "gr_zero_idle_bursts.h"
#include "src/config_mmdvm.h"

class gr_mod_mmdvm;

typedef std::shared_ptr<gr_mod_mmdvm> gr_mod_mmdvm_sptr;
gr_mod_mmdvm_sptr make_gr_mod_mmdvm(int sps=10, int samp_rate=MMDVM_SAMPLE_RATE, int carrier_freq=1700,
                                    int filter_width=8000);


class gr_mod_mmdvm : public gr::hier_block2
{
public:
    explicit gr_mod_mmdvm(int sps=10, int samp_rate=MMDVM_SAMPLE_RATE, int carrier_freq=1700,
                             int filter_width=8000);
    void set_bb_gain(float value);

private:

    gr::analog::frequency_modulator_fc::sptr _fm_modulator;
    gr::filter::rational_resampler_ccf::sptr _resampler;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::blocks::multiply_const_cc::sptr _bb_gain;
    gr::blocks::multiply_const_ff::sptr _audio_amplify;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::blocks::short_to_float::sptr _short_to_float;
    gr_zero_idle_bursts_sptr _zero_idle_bursts;


    int _samp_rate;
    int _sps;
    int _carrier_freq;
    int _filter_width;

};
#endif // GR_MOD_MMDVM_H
