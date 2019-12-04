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

#ifndef GR_DEMOD_WBFM_SDR_H
#define GR_DEMOD_WBFM_SDR_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/agc2_ff.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/blocks/multiply_const_ff.h>


class gr_demod_wbfm_sdr;

typedef boost::shared_ptr<gr_demod_wbfm_sdr> gr_demod_wbfm_sdr_sptr;
gr_demod_wbfm_sdr_sptr make_gr_demod_wbfm_sdr(int sps=125, int samp_rate=250000, int carrier_freq=1700,
                                          int filter_width=8000);

class gr_demod_wbfm_sdr : public gr::hier_block2
{
public:
    explicit gr_demod_wbfm_sdr(std::vector<int> signature, int sps=4, int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=1800);


    void set_squelch(int value);
    void set_filter_width(int filter_width);

private:
    gr::analog::quadrature_demod_cf::sptr _fm_demod;
    gr::analog::pwr_squelch_cc::sptr _squelch;
    gr::blocks::multiply_const_ff::sptr _amplify;
    gr::filter::rational_resampler_base_fff::sptr _audio_resampler;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::filter::fft_filter_fff::sptr _deemphasis_filter;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    int _target_samp_rate;

};

#endif // GR_DEMOD_WBFM_SDR_H
