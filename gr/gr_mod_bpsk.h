#ifndef GR_MOD_BPSK_H
#define GR_MOD_BPSK_H

#include <QObject>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_source_b.h>
#include <gnuradio/blocks/packed_to_unpacked_bb.h>
#include <gnuradio/audio/sink.h>
#include <gnuradio/endianness.h>
#include <gnuradio/digital/chunks_to_symbols_bc.h>
#include <gnuradio/blocks/repeat.h>
#include <gnuradio/filter/interp_fir_filter_fff.h>
#include <gnuradio/filter/interp_fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/sig_source_waveform.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/multiply_const_cc.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/digital/diff_encoder_bb.h>
#include <vector>
#include "gr_vector_source.h"

class gr_mod_bpsk : public QObject
{
    Q_OBJECT
public:
    explicit gr_mod_bpsk(QObject *parent = 0, int sps=4, int samp_rate=8000, int carrier_freq=1600, int filter_width=1100, float mod_index=1);

signals:


public slots:
    void start();
    void stop();
    int setData(std::vector<u_int8_t> *data);

private:
    gr::top_block_sptr _top_block;
    gr_vector_source_sptr _vector_source;
    gr::blocks::packed_to_unpacked_bb::sptr _packed_to_unpacked;
    gr::digital::chunks_to_symbols_bc::sptr _chunks_to_symbols;
    gr::blocks::repeat::sptr _repeat;
    gr::blocks::multiply_cc::sptr _multiply;
    gr::analog::sig_source_c::sptr _signal_source;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::filter::fir_filter_ccf::sptr _band_pass_filter_1;
    gr::filter::fir_filter_ccf::sptr _band_pass_filter_2;
    gr::blocks::complex_to_real::sptr _complex_to_real;
    gr::audio::sink::sptr _audio_sink;
    gr::digital::diff_encoder_bb::sptr _diff_encoder;


    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _modulation_index;


};

#endif // GR_MOD_BPSK_H
