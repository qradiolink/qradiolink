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
#include <gnuradio/blocks/packed_to_unpacked.h>
#include <gnuradio/endianness.h>
#include <gnuradio/digital/chunks_to_symbols.h>
#include <gnuradio/digital/map_bb.h>
#include <gnuradio/blocks/pack_k_bits_bb.h>




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
    gr::filter::rational_resampler_base_fff::sptr _first_resampler;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::blocks::multiply_const_cc::sptr _bb_gain;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::blocks::multiply_ff::sptr _multiply;
    gr::blocks::packed_to_unpacked_bb::sptr _packed_to_unpacked;
    gr::digital::chunks_to_symbols_bf::sptr _chunks_to_symbols;
    gr::blocks::pack_k_bits_bb::sptr _packer;
    gr::digital::map_bb::sptr _map;


    int _samp_rate;
    int _sps;
    int _samples_per_symbol;
    int _carrier_freq;
    int _filter_width;

};

#endif // GR_MOD_M17_H
