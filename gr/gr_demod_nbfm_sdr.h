#ifndef GR_DEMOD_NBFM_SDR_H
#define GR_DEMOD_NBFM_SDR_H

#include <QObject>
#include <gnuradio/audio/sink.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/top_block.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/filter/fft_filter_ccf.h>
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

class gr_demod_nbfm_sdr : public QObject
{
    Q_OBJECT
public:
    explicit gr_demod_nbfm_sdr(gr::qtgui::sink_c::sptr fft_gui,
                               gr::qtgui::const_sink_c::sptr const_gui, gr::qtgui::number_sink::sptr rssi_gui,
                               QObject *parent = 0, int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=1200, float mod_index=1, float device_frequency=434000000,
                               float rf_gain=50, std::string device_args="rtl=0", std::string device_antenna="RX2", int freq_corr=0);

public slots:
    void start();
    void stop();
    void tune(long center_freq);
    void set_rx_sensitivity(float value);

private:
    gr::top_block_sptr _top_block;
    gr::audio::sink::sptr _audio_sink;
    gr::analog::sig_source_c::sptr _signal_source;
    gr::blocks::multiply_cc::sptr _multiply;
    gr::analog::quadrature_demod_cf::sptr _fm_demod;
    gr::filter::pfb_arb_resampler_ccf::sptr _resampler;
    gr::filter::fft_filter_ccf::sptr _filter;
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

#endif // GR_DEMOD_NBFM_SDR_H
