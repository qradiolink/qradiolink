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

#ifndef GR_DEMOD_MMDVM_H
#define GR_DEMOD_MMDVM_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base.h>
#include <gnuradio/filter/rational_resampler_base.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/blocks/float_to_short.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/throttle.h>

class gr_demod_mmdvm;

typedef boost::shared_ptr<gr_demod_mmdvm> gr_demod_mmdvm_sptr;
gr_demod_mmdvm_sptr make_gr_demod_mmdvm(int sps=125, int samp_rate=1000000, int carrier_freq=1700,
                                          int filter_width=5000);

class gr_demod_mmdvm : public gr::hier_block2
{
public:
    explicit gr_demod_mmdvm(std::vector<int> signature, int sps=125, int samp_rate=1000000, int carrier_freq=1600,
                               int filter_width=5000);

    void set_squelch(int value);
    void set_filter_width(int filter_width);


private:
    gr::blocks::float_to_short::sptr _float_to_short;
    gr::analog::quadrature_demod_cf::sptr _fm_demod;
    gr::analog::pwr_squelch_cc::sptr _squelch;
    gr::blocks::multiply_const_ff::sptr _level_control;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::filter::rational_resampler_base_fff::sptr _audio_resampler;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::blocks::throttle::sptr _throttle;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _target_samp_rate;


};


#endif // GR_DEMOD_MMDVM_H
