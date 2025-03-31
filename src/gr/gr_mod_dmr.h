#ifndef GR_MOD_DMR_H
#define GR_MOD_DMR_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/rational_resampler.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/packed_to_unpacked.h>
#include <gnuradio/endianness.h>
#include <gnuradio/digital/chunks_to_symbols.h>
#include <gnuradio/digital/map_bb.h>
#include <gnuradio/blocks/pack_k_bits_bb.h>
#include "src/gr/gr_zero_idle_bursts.h"

class gr_mod_dmr;

typedef std::shared_ptr<gr_mod_dmr> gr_mod_dmr_sptr;
gr_mod_dmr_sptr make_gr_mod_dmr(int sps=125, int samp_rate=1000000, int carrier_freq=1700,
                                          int filter_width=5000);

class gr_mod_dmr : public gr::hier_block2
{
public:
    explicit gr_mod_dmr(int sps=125, int samp_rate=1000000, int carrier_freq=1700,
                             int filter_width=9000);
    void set_bb_gain(float value);

private:

    gr::analog::frequency_modulator_fc::sptr _fm_modulator;
    gr::filter::rational_resampler_ccf::sptr _resampler;
    gr::filter::rational_resampler_fff::sptr _first_resampler;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::blocks::multiply_const_cc::sptr _bb_gain;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::blocks::multiply_ff::sptr _multiply;
    gr::blocks::packed_to_unpacked_bb::sptr _packed_to_unpacked;
    gr::digital::chunks_to_symbols_bf::sptr _chunks_to_symbols;
    gr::blocks::multiply_const_ff::sptr _scale_pulses;
    gr::blocks::pack_k_bits_bb::sptr _packer;
    gr::digital::map_bb::sptr _map;
    gr_zero_idle_bursts_sptr _zero_idle;


    int _samp_rate;
    int _sps;
    int _samples_per_symbol;
    int _carrier_freq;
    int _filter_width;

};

#endif // GR_MOD_DMR_H
