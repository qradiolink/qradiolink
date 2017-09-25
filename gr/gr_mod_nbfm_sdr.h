#ifndef GR_MOD_NBFM_SDR_H
#define GR_MOD_NBFM_SDR_H

#include <QObject>
#include <gnuradio/top_block.h>
#include <gnuradio/audio/source.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/blocks/multiply_const_cc.h>
#include <gnuradio/blocks/multiply_ff.h>
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/analog/agc2_ff.h>
#include <osmosdr/sink.h>

class gr_mod_nbfm_sdr : public QObject
{
    Q_OBJECT
public:
    explicit gr_mod_nbfm_sdr(QObject *parent = 0, int samp_rate=250000, int carrier_freq=1700,
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
    gr::analog::frequency_modulator_fc::sptr _fm_modulator;
    gr::filter::pfb_arb_resampler_ccf::sptr _resampler;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::filter::fft_filter_fff::sptr _audio_filter;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::analog::sig_source_f::sptr _signal_source;
    gr::blocks::multiply_ff::sptr _multiply;
    gr::analog::agc2_ff::sptr _agc;

    osmosdr::sink::sptr _osmosdr_sink;

    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _modulation_index;
    float _device_frequency;
};

#endif // GR_MOD_NBFM_SDR_H
