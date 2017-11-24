#include "gr_mod_2fsk_sdr.h"

gr_mod_2fsk_sdr::gr_mod_2fsk_sdr(QObject *parent, int sps, int samp_rate, int carrier_freq,
                                 int filter_width, float mod_index, float device_frequency, float rf_gain,
                                 std::string device_args, std::string device_antenna, int freq_corr) :
    QObject(parent)
{
    std::vector<float> constellation;
    constellation.push_back(-1);
    constellation.push_back(1);
    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);

    _device_frequency = device_frequency;
    _samples_per_symbol = sps;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("2fsk modulator sdr");
    _vector_source = make_gr_vector_source();
    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _scrambler = gr::digital::scrambler_bb::make(0x8A, 0x7F ,7);

    _diff_encoder = gr::digital::diff_encoder_bb::make(2);
    _map = gr::digital::map_bb::make(map);

    _chunks_to_symbols = gr::digital::chunks_to_symbols_bf::make(constellation);
    _freq_modulator = gr::analog::frequency_modulator_fc::make((2*M_PI/2)/(_samples_per_symbol));
    _repeat = gr::blocks::repeat::make(4, _samples_per_symbol);
    _amplify = gr::blocks::multiply_const_cc::make(0.3,1);
    _filter = gr::filter::fft_filter_ccf::make(
                1,gr::filter::firdes::low_pass(
                    1, _samp_rate, _filter_width, 600,gr::filter::firdes::WIN_HAMMING));
    _osmosdr_sink = osmosdr::sink::make(device_args);
    _osmosdr_sink->set_sample_rate(_samp_rate);
    _osmosdr_sink->set_antenna(device_antenna);
    _osmosdr_sink->set_bandwidth(_samp_rate*20);

    _osmosdr_sink->set_center_freq(_device_frequency);

    osmosdr::gain_range_t range = _osmosdr_sink->get_gain_range();
    if (!range.empty())
    {
        double gain =  range.start() + rf_gain*(range.stop()-range.start());
        _osmosdr_sink->set_gain(gain);
    }
    _top_block->connect(_vector_source,0,_packed_to_unpacked,0);
    _top_block->connect(_packed_to_unpacked,0,_scrambler,0);
    //_top_block->connect(_scrambler,0,_map,0);
    //_top_block->connect(_map,0,_diff_encoder,0);
    _top_block->connect(_scrambler,0,_chunks_to_symbols,0);
    _top_block->connect(_chunks_to_symbols,0,_repeat,0);

    _top_block->connect(_repeat,0,_freq_modulator,0);
    _top_block->connect(_freq_modulator,0,_amplify,0);
    _top_block->connect(_amplify,0,_filter,0);

    _top_block->connect(_filter,0,_osmosdr_sink,0);
}

gr_mod_2fsk_sdr::~gr_mod_2fsk_sdr()
{
    _osmosdr_sink.reset();
}

void gr_mod_2fsk_sdr::start()
{
    _top_block->start();
}

void gr_mod_2fsk_sdr::stop()
{
    _top_block->stop();
    _top_block->wait();
}

int gr_mod_2fsk_sdr::setData(std::vector<u_int8_t> *data)
{
    return _vector_source->set_data(data);

}

void gr_mod_2fsk_sdr::tune(long center_freq)
{
    _device_frequency = center_freq;
    _osmosdr_sink->set_center_freq(_device_frequency);
}

void gr_mod_2fsk_sdr::set_power(float dbm)
{
    osmosdr::gain_range_t range = _osmosdr_sink->get_gain_range();
    if (!range.empty())
    {
        double gain =  range.start() + dbm*(range.stop()-range.start());
        _osmosdr_sink->set_gain(gain);
    }
}

