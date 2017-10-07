#include "gr_demod_2fsk_sdr.h"

gr_demod_2fsk_sdr::gr_demod_2fsk_sdr(gr::qtgui::sink_c::sptr fft_gui, gr::qtgui::const_sink_c::sptr const_gui,
                                     gr::qtgui::number_sink::sptr rssi_gui, QObject *parent,
                                     int sps, int samp_rate, int carrier_freq,
                                     int filter_width, float mod_index, float device_frequency, float rf_gain,
                                     std::string device_args, std::string device_antenna, int freq_corr) :
    QObject(parent)
{
    _target_samp_rate = 40000;
    _rssi = rssi_gui;
    _device_frequency = device_frequency;
    _samples_per_symbol = sps*4/25;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("fsk demodulator sdr");

    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);

    std::vector<float> taps = gr::filter::firdes::low_pass(32, _samp_rate, _filter_width, 12000);
    std::vector<float> symbol_filter_taps = gr::filter::firdes::low_pass(1.0,
                                 _target_samp_rate, _target_samp_rate*0.75/_samples_per_symbol, _target_samp_rate*0.25/_samples_per_symbol);
    _resampler = gr::filter::rational_resampler_base_ccf::make(1, 25, taps);
    _signal_source = gr::analog::sig_source_c::make(_samp_rate,gr::analog::GR_COS_WAVE,-25000,1);
    _multiply = gr::blocks::multiply_cc::make();
    //_freq_transl_filter = gr::filter::freq_xlating_fir_filter_ccf::make(
    //            1,gr::filter::firdes::low_pass(
    //                1, _target_samp_rate, 2*_filter_width, 250000, gr::filter::firdes::WIN_HAMMING), 25000,
    //            _target_samp_rate);
    _filter = gr::filter::fft_filter_ccf::make(1, gr::filter::firdes::low_pass(
                                1, _target_samp_rate, _filter_width,2000,gr::filter::firdes::WIN_HAMMING) );

    _upper_filter = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, -_filter_width,600,600,gr::filter::firdes::WIN_HAMMING) );
    _lower_filter = gr::filter::fft_filter_ccc::make(1, gr::filter::firdes::complex_band_pass(
                                1, _target_samp_rate, -600,_filter_width,600,gr::filter::firdes::WIN_HAMMING) );
    _mag_squared_lower = gr::blocks::complex_to_mag_squared::make();
    _mag_squared_upper = gr::blocks::complex_to_mag_squared::make();
    _divide = gr::blocks::divide_ff::make();
    _add = gr::blocks::add_const_ff::make(-0.5);
    _threshhold = gr::blocks::threshold_ff::make(0.5,1.5);
    _float_to_complex = gr::blocks::float_to_complex::make();
    _symbol_filter = gr::filter::fft_filter_ccf::make(1,symbol_filter_taps);
    float gain_mu = 0.025;
    _clock_recovery = gr::digital::clock_recovery_mm_cc::make(_samples_per_symbol, 0.025*gain_mu*gain_mu, 0.5, gain_mu,
                                                              0.015);
    _complex_to_real = gr::blocks::complex_to_real::make();
    _binary_slicer = gr::digital::binary_slicer_fb::make();
    _diff_decoder = gr::digital::diff_decoder_bb::make(2);
    _map = gr::digital::map_bb::make(map);
    _descrambler = gr::digital::descrambler_bb::make(0x8A, 0x7F ,7);
    _vector_sink = make_gr_vector_sink();

    _mag_squared = gr::blocks::complex_to_mag_squared::make();
    _single_pole_filter = gr::filter::single_pole_iir_filter_ff::make(0.04);
    _log10 = gr::blocks::nlog10_ff::make();
    _multiply_const_ff = gr::blocks::multiply_const_ff::make(10);
    _moving_average = gr::blocks::moving_average_ff::make(25000,1,2000);
    _add_const = gr::blocks::add_const_ff::make(-110);

    _osmosdr_source = osmosdr::source::make(device_args);
    _osmosdr_source->set_center_freq(_device_frequency-25000);
    _osmosdr_source->set_sample_rate(_samp_rate);
    _osmosdr_source->set_freq_corr(freq_corr);
    _osmosdr_source->set_gain_mode(false);
    _osmosdr_source->set_dc_offset_mode(0);
    _osmosdr_source->set_iq_balance_mode(0);
    _osmosdr_source->set_antenna(device_antenna);
    osmosdr::gain_range_t range = _osmosdr_source->get_gain_range();
    if (!range.empty())
    {
        double gain =  range.start() + rf_gain*(range.stop()-range.start());
        _osmosdr_source->set_gain(gain);
    }
    else
    {
        _osmosdr_source->set_gain_mode(true);
    }
    _constellation = const_gui;
    _fft_gui = fft_gui;
    _top_block->connect(_osmosdr_source,0,_multiply,0);
    _top_block->connect(_signal_source,0,_multiply,1);
    _top_block->connect(_multiply,0,_fft_gui,0);
    _top_block->connect(_multiply,0,_resampler,0);
    _top_block->connect(_resampler,0,_filter,0);
    _top_block->connect(_filter,0,_lower_filter,0);
    _top_block->connect(_filter,0,_upper_filter,0);
    _top_block->connect(_lower_filter,0,_mag_squared_lower,0);
    _top_block->connect(_upper_filter,0,_mag_squared_upper,0);
    _top_block->connect(_mag_squared_lower,0,_divide,1);
    _top_block->connect(_mag_squared_upper,0,_divide,0);
    _top_block->connect(_divide,0,_threshhold,0);
    _top_block->connect(_threshhold,0,_add,0);
    _top_block->connect(_add,0,_float_to_complex,0);
    _top_block->connect(_float_to_complex,0,_symbol_filter,0);
    _top_block->connect(_symbol_filter,0,_clock_recovery,0);
    _top_block->connect(_clock_recovery,0,_constellation,0);
    _top_block->connect(_clock_recovery,0,_complex_to_real,0);
    _top_block->connect(_complex_to_real,0,_binary_slicer,0);
    //_top_block->connect(_binary_slicer,0,_diff_decoder,0);
    //_top_block->connect(_diff_decoder,0,_map,0);
    _top_block->connect(_binary_slicer,0,_descrambler,0);
    _top_block->connect(_descrambler,0,_vector_sink,0);

    _top_block->connect(_filter,0,_mag_squared,0);
    _top_block->connect(_mag_squared,0,_moving_average,0);
    _top_block->connect(_moving_average,0,_single_pole_filter,0);
    _top_block->connect(_single_pole_filter,0,_log10,0);
    _top_block->connect(_log10,0,_multiply_const_ff,0);
    _top_block->connect(_multiply_const_ff,0,_add_const,0);
    _top_block->connect(_add_const,0,_rssi,0);
}

void gr_demod_2fsk_sdr::start()
{
    _top_block->start();
}

void gr_demod_2fsk_sdr::stop()
{
    _top_block->stop();
    _top_block->wait();
}

std::vector<unsigned char>* gr_demod_2fsk_sdr::getData()
{
    std::vector<unsigned char> *data = _vector_sink->get_data();
    return data;
}

void gr_demod_2fsk_sdr::tune(long center_freq)
{
    _device_frequency = center_freq;
    _osmosdr_source->set_center_freq(_device_frequency-25000);
}

void gr_demod_2fsk_sdr::set_rx_sensitivity(float value)
{
    osmosdr::gain_range_t range = _osmosdr_source->get_gain_range();
    if (!range.empty())
    {
        double gain =  range.start() + value*(range.stop()-range.start());
        _osmosdr_source->set_gain(gain);
    }
}
