// Written by Adrian Musceac YO8RZZ , started October 2024.
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

#ifndef GR_DEMOD_DMR_H
#define GR_DEMOD_DMR_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/digital/symbol_sync_ff.h>
#include <gnuradio/analog/phase_modulator_fc.h>
#include <gnuradio/filter/fir_filter.h>
#include <gnuradio/blocks/complex_to_float.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/blocks/interleave.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/digital/map_bb.h>
#include <gnuradio/blocks/pack_k_bits_bb.h>
#include <gnuradio/blocks/unpack_k_bits_bb.h>
#include <gnuradio/top_block.h>


class gr_demod_dmr;

typedef std::shared_ptr<gr_demod_dmr> gr_demod_dmr_sptr;
gr_demod_dmr_sptr make_gr_demod_dmr(int sps, int samp_rate);

class gr_demod_dmr : public gr::hier_block2
{
public:
    explicit gr_demod_dmr(std::vector<int> signature, int sps, int samp_rate);

private:
    gr::analog::quadrature_demod_cf::sptr _fm_demod;
    gr::blocks::multiply_const_ff::sptr _level_control;
    gr::filter::rational_resampler_ccf::sptr _resampler;
    gr::blocks::float_to_complex::sptr _float_to_complex_corr;
    gr::blocks::complex_to_float::sptr _complex_to_float_corr;
    gr::filter::fft_filter_fff::sptr _symbol_filter;
    gr::digital::symbol_sync_ff::sptr _symbol_sync;
    gr::blocks::complex_to_float::sptr _complex_to_float;
    gr::blocks::interleave::sptr _interleave;
    gr::analog::phase_modulator_fc::sptr _phase_mod;
    gr::digital::binary_slicer_fb::sptr _slicer;
    gr::digital::map_bb::sptr _symbol_map;
    gr::blocks::pack_k_bits_bb::sptr _packer;
    gr::blocks::unpack_k_bits_bb::sptr _unpacker;
    int _sps;
    int _samp_rate;
    int _filter_width;
    float _target_samp_rate;

};


#endif // GR_DEMOD_DMR_H
