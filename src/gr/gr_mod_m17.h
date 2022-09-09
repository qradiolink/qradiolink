// Written by Adrian Musceac YO8RZZ , started September 2022.
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

#ifndef GR_MOD_M17_H
#define GR_MOD_M17_H


#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/rational_resampler_base.h>
#include <gnuradio/filter/rational_resampler_base.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/multiply.h>




class gr_mod_m17;

typedef boost::shared_ptr<gr_mod_m17> gr_mod_m17_sptr;
gr_mod_m17_sptr make_gr_mod_m17(int sps=125, int samp_rate=1000000, int carrier_freq=1700,
                                          int filter_width=6250);

class gr_mod_m17 : public gr::hier_block2
{
public:
    explicit gr_mod_m17(int sps=125, int samp_rate=1000000, int carrier_freq=1700,
                             int filter_width=6250);
    void set_filter_width(int filter_width);
    void set_bb_gain(float value);

private:

    gr::analog::frequency_modulator_fc::sptr _fm_modulator;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::blocks::multiply_const_cc::sptr _bb_gain;
    gr::blocks::multiply_const_ff::sptr _audio_amplify;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::blocks::multiply_ff::sptr _multiply;


    int _samp_rate;
    int _sps;
    int _carrier_freq;
    int _filter_width;

};

#endif // GR_MOD_M17_H
