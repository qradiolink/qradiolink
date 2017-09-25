#ifndef GR_MOD_SSB_SDR_H
#define GR_MOD_SSB_SDR_H

#include <QObject>
#include <gnuradio/top_block.h>
#include <gnuradio/audio/source.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/blocks/multiply_const_cc.h>
#include <gnuradio/blocks/multiply_ff.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/blocks/delay.h>
#include <osmosdr/sink.h>

class gr_mod_ssb_sdr : public QObject
{
    Q_OBJECT
public:
    explicit gr_mod_ssb_sdr(QObject *parent = 0, int samp_rate=250000, int carrier_freq=1700,
                            int filter_width=1200, float mod_index=1, float device_frequency=434000000,
                            float rf_gain=70, std::string device_args="uhd", std::string device_antenna="TX/RX", int freq_corr=0);

public slots:
    void start();
    void stop();
    void tune(long center_freq);
    void set_power(int dbm);


private:
    gr::top_block_sptr _top_block;
    gr::audio::source::sptr _audio_source;
    gr::filter::pfb_arb_resampler_ccf::sptr _resampler;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::filter::fft_filter_fff::sptr _audio_filter;
    gr::filter::fft_filter_ccc::sptr _filter;
    gr::analog::sig_source_f::sptr _signal_source;
    gr::analog::sig_source_c::sptr _carrier_suppress;
    gr::blocks::multiply_ff::sptr _multiply;
    gr::blocks::multiply_cc::sptr _multiply2;
    gr::blocks::float_to_complex::sptr _float_to_complex;
    gr::blocks::delay::sptr _delay;

    osmosdr::sink::sptr _osmosdr_sink;

    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _modulation_index;
    float _device_frequency;
};

#endif // GR_MOD_SSB_SDR_H
