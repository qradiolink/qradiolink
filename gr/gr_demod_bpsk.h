#ifndef GR_DEMOD_BPSK_H
#define GR_DEMOD_BPSK_H

#include <QObject>
#include <gnuradio/audio/source.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/top_block.h>
#include <gnuradio/endianness.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/sig_source_waveform.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/digital/clock_recovery_mm_cc.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/blocks/unpacked_to_packed_bb.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/digital/costas_loop_cc.h>
#include <gnuradio/digital/diff_decoder_bb.h>
#include <gnuradio/analog/feedforward_agc_cc.h>
#include <vector>
#include "gr_vector_sink.h"

class gr_demod_bpsk : public QObject
{
    Q_OBJECT
public:
    explicit gr_demod_bpsk(QObject *parent = 0, int sps=4, int samp_rate=8000, int carrier_freq=1600, int filter_width=1100, float mod_index=1);

signals:

public slots:
    void start();
    void stop();
    std::vector<unsigned char> *getData();

private:
    gr::top_block_sptr _top_block;
    gr_vector_sink_sptr _vector_sink;
    gr::blocks::unpacked_to_packed_bb::sptr _unpacked_to_packed;
    gr::blocks::multiply_cc::sptr _multiply;
    gr::analog::sig_source_c::sptr _signal_source;
    gr::filter::fir_filter_ccf::sptr _band_pass_filter_1;
    gr::filter::fir_filter_ccf::sptr _band_pass_filter_2;
    gr::blocks::complex_to_real::sptr _complex_to_real;
    gr::blocks::float_to_complex::sptr _float_to_complex;
    gr::audio::source::sptr _audio_source;
    gr::analog::feedforward_agc_cc::sptr _agc;
    gr::digital::clock_recovery_mm_cc::sptr _clock_recovery;
    gr::digital::binary_slicer_fb::sptr _binary_slicer;
    gr::digital::costas_loop_cc::sptr _costas_loop;
    gr::digital::diff_decoder_bb::sptr _diff_decoder;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _modulation_index;

};

#endif // GR_DEMOD_BPSK_H
