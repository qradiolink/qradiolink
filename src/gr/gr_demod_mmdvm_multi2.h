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

#ifndef GR_DEMOD_MMDVM_MULTI2_H
#define GR_DEMOD_MMDVM_MULTI2_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/blocks/float_to_short.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/stream_to_streams.h>
#include <gnuradio/filter/pfb_channelizer_ccf.h>
#include "src/bursttimer.h"
#include "gr_mmdvm_sink.h"
#include "rssi_tag_block.h"

class gr_demod_mmdvm_multi2;

typedef std::shared_ptr<gr_demod_mmdvm_multi2> gr_demod_mmdvm_multi2_sptr;
gr_demod_mmdvm_multi2_sptr make_gr_demod_mmdvm_multi2(BurstTimer *burst_timer, int num_channels=3,
                                                    int channel_separation=25000, bool use_tdma=true,
                                                    int sps=125, int samp_rate=MMDVM_SAMPLE_RATE, int carrier_freq=1700,
                                                    int filter_width=5000);

class gr_demod_mmdvm_multi2 : public gr::hier_block2
{
public:
    explicit gr_demod_mmdvm_multi2(BurstTimer *burst_timer, int num_channels=3, int channel_separation=25000,
                                  bool use_tdma=true,
                                  int sps=125, int samp_rate=MMDVM_SAMPLE_RATE, int carrier_freq=1600,
                                    int filter_width=5000);

    void set_filter_width(int filter_width);
    void calibrate_rssi(float level);


private:
    gr::blocks::float_to_short::sptr _float_to_short[MAX_MMDVM_CHANNELS];
    gr::analog::quadrature_demod_cf::sptr _fm_demod[MAX_MMDVM_CHANNELS];
    gr::blocks::multiply_const_ff::sptr _level_control[MAX_MMDVM_CHANNELS];
    gr::filter::rational_resampler_ccf::sptr _resampler[MAX_MMDVM_CHANNELS];
    gr::filter::fft_filter_ccf::sptr _filter[MAX_MMDVM_CHANNELS];
    gr_mmdvm_sink_sptr _mmdvm_sink;
    rssi_tag_block_sptr _rssi_tag_block[MAX_MMDVM_CHANNELS];
    gr::blocks::null_sink::sptr _null_sink[10];
    gr::filter::pfb_channelizer_ccf::sptr _channelizer;
    gr::blocks::stream_to_streams::sptr _stream_to_streams;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    int _num_channels;
    bool _use_tdma;

};


#endif // GR_DEMOD_MMDVM_MULTI2_H
