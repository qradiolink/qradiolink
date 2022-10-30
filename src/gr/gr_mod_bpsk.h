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

#ifndef GR_MOD_BPSK_H
#define GR_MOD_BPSK_H


#include <QObject>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/packed_to_unpacked.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/endianness.h>
#include <gnuradio/digital/chunks_to_symbols.h>
#include <gnuradio/blocks/repeat.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/sig_source_waveform.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/digital/scrambler_bb.h>
#include <gnuradio/fec/cc_encoder.h>
#include <gnuradio/fec/encoder.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/rational_resampler.h>


class gr_mod_bpsk;

typedef std::shared_ptr<gr_mod_bpsk> gr_mod_bpsk_sptr;
gr_mod_bpsk_sptr make_gr_mod_bpsk(int sps=125, int samp_rate=250000, int carrier_freq=1700,
                                          int filter_width=8000);

class gr_mod_bpsk : public gr::hier_block2
{

public:
    explicit gr_mod_bpsk(int sps=125, int samp_rate=250000, int carrier_freq=1700,
                             int filter_width=8000);
    void set_bb_gain(float value);

private:
    gr::blocks::packed_to_unpacked_bb::sptr _packed_to_unpacked;
    gr::digital::chunks_to_symbols_bc::sptr _chunks_to_symbols;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::blocks::multiply_const_cc::sptr _bb_gain;
    gr::fec::encoder::sptr _encode_ccsds;
    gr::digital::scrambler_bb::sptr _scrambler;
    gr::filter::rational_resampler_ccf::sptr _resampler;
    gr::filter::rational_resampler_ccf::sptr _resampler2;



    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;

};

#endif // GR_MOD_BPSK_H
