#ifndef GR_DEMOD_2FSK_SDR_H
#define GR_DEMOD_2FSK_SDR_H

#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/top_block.h>
#include <gnuradio/endianness.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/digital/clock_recovery_mm_cc.h>
#include <gnuradio/blocks/unpack_k_bits_bb.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/digital/diff_decoder_bb.h>
#include <gnuradio/blocks/multiply_const_cc.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/blocks/divide_ff.h>
#include <gnuradio/blocks/threshold_ff.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/digital/map_bb.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_ccc.h>
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
#include <gnuradio/blocks/copy.h>
#include <osmosdr/source.h>
#include <vector>
#include "gr_vector_sink.h"
#include <QObject>

class gr_demod_2fsk_sdr : public QObject
{
    Q_OBJECT
public:
    explicit gr_demod_2fsk_sdr(gr::qtgui::sink_c::sptr fft_gui, gr::qtgui::const_sink_c::sptr const_gui,
                               gr::qtgui::number_sink::sptr rssi_gui, QObject *parent = 0, int sps=4,
                               int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=1800, float mod_index=1, float device_frequency=434000000,
                               float rf_gain=50, std::string device_args="rtl=0", std::string device_antenna="RX2", int freq_corr=0);

signals:

public slots:
    void start();
    void stop();
    std::vector<unsigned char> *getData();
    void tune(long center_freq);
    void set_rx_sensitivity(float value);
    void enable_gui(bool value);

private:
    gr::top_block_sptr _top_block;
    gr_vector_sink_sptr _vector_sink;
    gr::analog::sig_source_c::sptr _signal_source;
    gr::blocks::multiply_cc::sptr _multiply;
    gr::blocks::multiply_const_cc::sptr _multiply_symbols;
    gr::blocks::float_to_complex::sptr _float_to_complex;
    gr::filter::fft_filter_ccf::sptr _symbol_filter;
    gr::digital::clock_recovery_mm_cc::sptr _clock_recovery;
    gr::digital::diff_decoder_bb::sptr _diff_decoder;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::digital::map_bb::sptr _map;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::filter::fft_filter_ccc::sptr _lower_filter;
    gr::filter::fft_filter_ccc::sptr _upper_filter;
    gr::blocks::complex_to_mag_squared::sptr _mag_squared_lower;
    gr::blocks::complex_to_mag_squared::sptr _mag_squared_upper;
    gr::blocks::divide_ff::sptr _divide;
    gr::blocks::add_const_ff::sptr _add;
    gr::blocks::threshold_ff::sptr _threshhold;
    gr::digital::binary_slicer_fb::sptr _binary_slicer;
    gr::blocks::complex_to_real::sptr _complex_to_real;
    gr::digital::descrambler_bb::sptr _descrambler;
    gr::qtgui::const_sink_c::sptr _constellation;
    gr::qtgui::sink_c::sptr _fft_gui;
    gr::blocks::copy::sptr _rssi_valve;
    gr::blocks::copy::sptr _fft_valve;
    gr::blocks::copy::sptr _const_valve;
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

#endif // GR_DEMOD_2FSK_SDR_H
