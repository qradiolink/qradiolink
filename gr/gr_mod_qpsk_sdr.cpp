// Written by Adrian Musceac YO8RZZ at gmail dot com, started March 2016.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "gr_mod_qpsk_sdr.h"

gr_mod_qpsk_sdr::gr_mod_qpsk_sdr(QObject *parent, int sps, int samp_rate, int carrier_freq,
                                 int filter_width, float mod_index, float device_frequency, float rf_gain) :
    QObject(parent)
{
    //gr::digital::constellation_qpsk::sptr constellation = gr::digital::constellation_qpsk::make();
    //std::vector<gr_complex> constellation_points = constellation->points();
    std::vector<gr_complex> constellation;
    constellation.push_back(-0.707-0.707j);
    constellation.push_back(-0.707+0.707j);
    constellation.push_back(0.707+0.707j);
    constellation.push_back(0.707-0.707j);

    std::vector<int> map;
    map.push_back(0);
    map.push_back(1);
    map.push_back(2);
    map.push_back(3);

    const std::string device_args = "uhd";
    _device_frequency = device_frequency;
    _samples_per_symbol = sps;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("qpsk modulator sdr");
    _vector_source = make_gr_vector_source();
    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _packer = gr::blocks::pack_k_bits_bb::make(2);
    _scrambler = gr::digital::scrambler_bb::make(0x8A, 0x7F ,7);
    //_packed_to_unpacked2 = gr::blocks::packed_to_unpacked_bb::make(2,gr::GR_MSB_FIRST);
    _diff_encoder = gr::digital::diff_encoder_bb::make(4);
    _map = gr::digital::map_bb::make(map);

    _chunks_to_symbols = gr::digital::chunks_to_symbols_bc::make(constellation);
    int nfilts = 32;
    std::vector<float> rrc_taps = gr::filter::firdes::root_raised_cosine(nfilts, nfilts,
                                                        1, 0.35, nfilts * 11 * _samples_per_symbol);
    _shaping_filter = gr::filter::pfb_arb_resampler_ccf::make(_samples_per_symbol, rrc_taps, nfilts);
    _repeat = gr::blocks::repeat::make(8, _samples_per_symbol);
    _amplify = gr::blocks::multiply_const_cc::make(0.2,1);
    _filter = gr::filter::fft_filter_ccf::make(
                1,gr::filter::firdes::low_pass(
                    1, _samp_rate, _filter_width, 400,gr::filter::firdes::WIN_HAMMING));
    _osmosdr_sink = osmosdr::sink::make(device_args);
    _osmosdr_sink->set_sample_rate(_samp_rate);
    _osmosdr_sink->set_antenna("TX/RX");

    _osmosdr_sink->set_center_freq(_device_frequency);

    _osmosdr_sink->set_gain(rf_gain);
    _top_block->connect(_vector_source,0,_packed_to_unpacked,0);
    _top_block->connect(_packed_to_unpacked,0,_scrambler,0);
    _top_block->connect(_scrambler,0,_packer,0);
    _top_block->connect(_packer,0,_map,0);
    _top_block->connect(_map,0,_diff_encoder,0);
    _top_block->connect(_diff_encoder,0,_chunks_to_symbols,0);
    _top_block->connect(_chunks_to_symbols,0,_repeat,0);

    _top_block->connect(_repeat,0,_amplify,0);
    _top_block->connect(_amplify,0,_filter,0);

    _top_block->connect(_filter,0,_osmosdr_sink,0);

}




void gr_mod_qpsk_sdr::start()
{
    _top_block->start();
}

void gr_mod_qpsk_sdr::stop()
{
    _top_block->stop();
    _top_block->wait();
}

int gr_mod_qpsk_sdr::setData(std::vector<u_int8_t> *data)
{
    return _vector_source->set_data(data);

}

void gr_mod_qpsk_sdr::tune(long center_freq)
{
    _device_frequency = center_freq;
    _osmosdr_sink->set_center_freq(_device_frequency);
}

void gr_mod_qpsk_sdr::set_power(int dbm)
{
    _osmosdr_sink->set_gain(dbm);
}
