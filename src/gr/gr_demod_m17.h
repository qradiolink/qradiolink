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

#ifndef GR_DEMOD_M17_H
#define GR_DEMOD_M17_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/digital/symbol_sync_ff.h>
#include <gnuradio/analog/phase_modulator_fc.h>
#include <gnuradio/blocks/complex_to_float.h>
#include <gnuradio/blocks/interleave.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/digital/map_bb.h>
#include <gnuradio/blocks/pack_k_bits_bb.h>
#include <gnuradio/blocks/unpack_k_bits_bb.h>


class gr_demod_m17;

typedef boost::shared_ptr<gr_demod_m17> gr_demod_m17_sptr;
gr_demod_m17_sptr make_gr_demod_m17(int sps=125, int samp_rate=1000000, int carrier_freq=1700,
                                          int filter_width=6250);

class gr_demod_m17 : public gr::hier_block2
{
public:
    explicit gr_demod_m17(std::vector<int> signature, int sps=4, int samp_rate=1000000, int carrier_freq=1600,
                               int filter_width=6250);

    void set_filter_width(int filter_width);


private:
    gr::analog::quadrature_demod_cf::sptr _fm_demod;
    gr::blocks::multiply_const_ff::sptr _level_control;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::filter::rational_resampler_base_fff::sptr _audio_resampler;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::filter::fft_filter_fff::sptr _audio_filter;
    gr::digital::symbol_sync_ff::sptr _symbol_sync;
    gr::filter::fft_filter_fff::sptr _symbol_filter;
    gr::analog::phase_modulator_fc::sptr _phase_mod;
    gr::blocks::complex_to_float::sptr _complex_to_float;
    gr::blocks::interleave::sptr _interleave;
    gr::digital::binary_slicer_fb::sptr _slicer;
    gr::digital::map_bb::sptr _symbol_map;
    gr::blocks::pack_k_bits_bb::sptr _packer;
    gr::blocks::unpack_k_bits_bb::sptr _unpacker;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    int _target_samp_rate;


};

#endif // GR_DEMOD_M17_H
