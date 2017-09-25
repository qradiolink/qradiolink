#include "gr_mod_ssb_sdr.h"

gr_mod_ssb_sdr::gr_mod_ssb_sdr(QObject *parent, int samp_rate, int carrier_freq, int filter_width,
                               float mod_index, float device_frequency, float rf_gain,
                               std::string device_args, std::string device_antenna, int freq_corr) :
    QObject(parent)
{

    _device_frequency = device_frequency;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("ssb modulator sdr");

    _audio_source = gr::audio::source::make(48000,"",false);
    _signal_source = gr::analog::sig_source_f::make(48000,gr::analog::GR_COS_WAVE, 0, 1);
    _carrier_suppress = gr::analog::sig_source_c::make(48000,gr::analog::GR_COS_WAVE,0,1);
    _delay = gr::blocks::delay::make(8,125000);
    _multiply = gr::blocks::multiply_ff::make();
    _multiply2 = gr::blocks::multiply_cc::make();
    _audio_filter = gr::filter::fft_filter_fff::make(
                1,gr::filter::firdes::low_pass(
                    1, 48000, _filter_width, 600, gr::filter::firdes::WIN_HAMMING));
    _float_to_complex = gr::blocks::float_to_complex::make();
    std::vector<float> interp_taps = gr::filter::firdes::low_pass(1, 48000,
                                                        10000, 10000);
    float rerate = (float)_samp_rate/48000.0;
    _resampler = gr::filter::pfb_arb_resampler_ccf::make(rerate, interp_taps, 16);
    _amplify = gr::blocks::multiply_const_cc::make(60,1);
    _filter = gr::filter::fft_filter_ccc::make(
                1,gr::filter::firdes::complex_band_pass(
                    1, _samp_rate, 300, _filter_width, 50, gr::filter::firdes::WIN_HAMMING));

    _osmosdr_sink = osmosdr::sink::make(device_args);
    _osmosdr_sink->set_sample_rate(_samp_rate);
    _osmosdr_sink->set_antenna(device_antenna);
    _osmosdr_sink->set_center_freq(_device_frequency);
    _osmosdr_sink->set_gain(rf_gain);



    _top_block->connect(_audio_source,0,_multiply,0);
    _top_block->connect(_signal_source,0,_multiply,1);
    _top_block->connect(_multiply,0,_audio_filter,0);

    _top_block->connect(_audio_filter,0,_float_to_complex,0);
    _top_block->connect(_float_to_complex,0,_resampler,0);
    _top_block->connect(_resampler,0,_multiply2,0);
    _top_block->connect(_carrier_suppress,0,_delay,0);
    _top_block->connect(_delay,0,_multiply2,1);
    _top_block->connect(_multiply2,0,_amplify,0);

    //_top_block->connect(_resampler,0,_amplify,0);
    _top_block->connect(_amplify,0,_filter,0);

    _top_block->connect(_filter,0,_osmosdr_sink,0);
}

void gr_mod_ssb_sdr::start()
{
    _top_block->start();
}

void gr_mod_ssb_sdr::stop()
{
    _top_block->stop();
    _top_block->wait();
}

void gr_mod_ssb_sdr::tune(long center_freq)
{
    _device_frequency = center_freq;
    _osmosdr_sink->set_center_freq(_device_frequency);
}

void gr_mod_ssb_sdr::set_power(int dbm)
{
    _osmosdr_sink->set_gain(dbm);
}
