#ifndef GR_DEMOD_BPSK_SDR_H
#define GR_DEMOD_BPSK_SDR_H

#include <QObject>
#include <gnuradio/audio/source.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/top_block.h>
#include <gnuradio/endianness.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/digital/clock_recovery_mm_cc.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/blocks/unpacked_to_packed_bb.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/digital/costas_loop_cc.h>
#include <gnuradio/digital/diff_decoder_bb.h>
#include <gnuradio/digital/cma_equalizer_cc.h>
#include <gnuradio/analog/agc2_cc.h>
#include <gnuradio/analog/agc2_ff.h>
#include <gnuradio/digital/fll_band_edge_cc.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/digital/descrambler_bb.h>
#include <gnuradio/qtgui/const_sink_c.h>
#include <gnuradio/qtgui/sink_c.h>
#include <gnuradio/qtgui/number_sink.h>
#include <gnuradio/blocks/complex_to_mag_squared.h>
#include <gnuradio/blocks/nlog10_ff.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/single_pole_iir_filter_ff.h>
#include <gnuradio/blocks/moving_average_ff.h>
#include <gnuradio/blocks/add_const_ff.h>
#include <osmosdr/source.h>
#include <vector>
#include "gr_vector_sink.h"

class gr_demod_bpsk_sdr : public QObject
{
    Q_OBJECT
public:
    explicit gr_demod_bpsk_sdr(gr::qtgui::sink_c::sptr fft_gui,
                               gr::qtgui::const_sink_c::sptr const_gui, gr::qtgui::number_sink::sptr rssi_gui, QObject *parent = 0, int sps=4, int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=1200, float mod_index=1, float device_frequency=434000000,
                               float rf_gain=50, std::string device_args="rtl=0", std::string device_antenna="RX2", int freq_corr=0);

signals:

public slots:
    void start();
    void stop();
    std::vector<unsigned char> *getData();
    void tune(long center_freq);
    void set_rx_sensitivity(float value);

private:
    gr::top_block_sptr _top_block;
    gr_vector_sink_sptr _vector_sink;
    gr::blocks::unpacked_to_packed_bb::sptr _unpacked_to_packed;
    gr::analog::sig_source_c::sptr _signal_source;
    gr::blocks::multiply_cc::sptr _multiply;
    gr::filter::freq_xlating_fir_filter_ccf::sptr _freq_transl_filter;
    gr::digital::cma_equalizer_cc::sptr _equalizer;
    gr::blocks::complex_to_real::sptr _complex_to_real;
    gr::analog::agc2_cc::sptr _agc;
    gr::analog::agc2_ff::sptr _agc2;
    gr::digital::fll_band_edge_cc::sptr _fll;
    gr::digital::clock_recovery_mm_cc::sptr _clock_recovery;
    gr::digital::binary_slicer_fb::sptr _binary_slicer;
    gr::digital::costas_loop_cc::sptr _costas_loop;
    gr::digital::diff_decoder_bb::sptr _diff_decoder;
    //gr::filter::pfb_arb_resampler_ccf::sptr _resampler;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::digital::descrambler_bb::sptr _descrambler;
    gr::qtgui::const_sink_c::sptr _constellation;
    gr::qtgui::sink_c::sptr _fft_gui;
    gr::blocks::complex_to_mag_squared::sptr _mag_squared;
    gr::blocks::nlog10_ff::sptr _log10;
    gr::filter::single_pole_iir_filter_ff::sptr _single_pole_filter;
    gr::blocks::multiply_const_ff::sptr _multiply_const_ff;
    gr::blocks::moving_average_ff::sptr _moving_average;
    gr::blocks::add_const_ff::sptr _add_const;
    gr::qtgui::number_sink::sptr _rssi;
    osmosdr::source::sptr _osmosdr_source;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _modulation_index;
    float _device_frequency;
    int _target_samp_rate;

};

#endif // GR_DEMOD_BPSK_SDR_H
