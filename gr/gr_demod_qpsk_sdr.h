#ifndef GR_DEMOD_QPSK_SDR_H
#define GR_DEMOD_QPSK_SDR_H

#include <QObject>
#include <gnuradio/audio/source.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/top_block.h>
#include <gnuradio/endianness.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/digital/clock_recovery_mm_cc.h>
#include <gnuradio/blocks/unpack_k_bits_bb.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/digital/costas_loop_cc.h>
#include <gnuradio/digital/diff_decoder_bb.h>
#include <gnuradio/digital/cma_equalizer_cc.h>
#include <gnuradio/analog/agc2_cc.h>
#include <gnuradio/digital/fll_band_edge_cc.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/digital/map_bb.h>
#include <gnuradio/digital/constellation.h>
#include <gnuradio/digital/constellation_decoder_cb.h>
//#include <gnuradio/qtgui/const_sink_c.h>
#include <osmosdr/source.h>
#include <vector>
#include "gr_vector_sink.h"

class gr_demod_qpsk_sdr : public QObject
{
    Q_OBJECT
public:
    explicit gr_demod_qpsk_sdr(QObject *parent = 0, int sps=4, int samp_rate=8000, int carrier_freq=1600,
                               int filter_width=1200, float mod_index=1, float device_frequency=434000000,
                               float rf_gain=50);

signals:

public slots:
    void start();
    void stop();
    std::vector<unsigned char> *getData();

private:
    gr::top_block_sptr _top_block;
    gr_vector_sink_sptr _vector_sink;
    gr::blocks::unpack_k_bits_bb::sptr _unpack;
    gr::filter::freq_xlating_fir_filter_ccf::sptr _filter;
    gr::digital::cma_equalizer_cc::sptr _equalizer;
    gr::analog::agc2_cc::sptr _agc;
    gr::digital::fll_band_edge_cc::sptr _fll;
    gr::digital::clock_recovery_mm_cc::sptr _clock_recovery;
    gr::digital::costas_loop_cc::sptr _costas_loop;
    gr::digital::diff_decoder_bb::sptr _diff_decoder;
    gr::filter::pfb_arb_resampler_ccf::sptr _resampler;
    gr::digital::map_bb::sptr _map;
    gr::digital::constellation_decoder_cb::sptr _constellation_receiver;
    //gr::qtgui::const_sink_c::sptr _constellation;
    osmosdr::source::sptr _osmosdr_source;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _modulation_index;
    float _device_frequency;
    int _target_samp_rate;

};

#endif // GR_DEMOD_QPSK_SDR_H
