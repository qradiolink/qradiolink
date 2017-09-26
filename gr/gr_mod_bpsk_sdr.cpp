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

#include "gr_mod_bpsk_sdr.h"
#include "QDebug"

gr_mod_bpsk_sdr::gr_mod_bpsk_sdr(QObject *parent, int sps, int samp_rate, int carrier_freq,
                                 int filter_width, float mod_index, float device_frequency, float rf_gain,
                                 std::string device_args, std::string device_antenna, int freq_corr) :
    QObject(parent)
{
    std::vector<gr_complex> constellation;
    constellation.push_back(-1);
    constellation.push_back(1);
    std::vector<int> polys;
    polys.push_back(109);
    polys.push_back(79);

    _device_frequency = device_frequency;
    _samples_per_symbol = sps;
    _samp_rate =samp_rate;
    _carrier_freq = carrier_freq;
    _filter_width = filter_width;
    _modulation_index = mod_index;
    _top_block = gr::make_top_block("bpsk modulator sdr");
    _vector_source = make_gr_vector_source();
    _packed_to_unpacked = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _scrambler = gr::digital::scrambler_bb::make(0x8A, 0x7F ,7);
    gr::fec::code::cc_encoder::sptr ccsds = gr::fec::code::cc_encoder::make(4096,7,2,polys,0,CC_TRUNCATED);
    _unpacked_to_packed = gr::blocks::unpacked_to_packed_bb::make(1,gr::GR_MSB_FIRST);
    _ccsds_encoder = gr::fec::encoder::make(ccsds,1,1);
    _packed_to_unpacked2 = gr::blocks::packed_to_unpacked_bb::make(1,gr::GR_MSB_FIRST);
    _diff_encoder = gr::digital::diff_encoder_bb::make(2);

    _chunks_to_symbols = gr::digital::chunks_to_symbols_bc::make(constellation);
    int nfilts = 32;
    std::vector<float> rrc_taps = gr::filter::firdes::root_raised_cosine(nfilts, nfilts,
                                                        1, 0.35, nfilts * 11 * _samples_per_symbol);
    _shaping_filter = gr::filter::pfb_arb_resampler_ccf::make(_samples_per_symbol, rrc_taps, nfilts);
    _repeat = gr::blocks::repeat::make(8, _samples_per_symbol);
    _amplify = gr::blocks::multiply_const_cc::make(0.33,1);
    _filter = gr::filter::fft_filter_ccf::make(
                1,gr::filter::firdes::low_pass(
                    1, _samp_rate, _filter_width, 600, gr::filter::firdes::WIN_HAMMING));

    _osmosdr_sink = osmosdr::sink::make(device_args);
    _osmosdr_sink->set_sample_rate(_samp_rate);
    _osmosdr_sink->set_antenna(device_antenna);
    _osmosdr_sink->set_center_freq(_device_frequency);
    _osmosdr_sink->set_gain(rf_gain);



    _top_block->connect(_vector_source,0,_packed_to_unpacked,0);
    _top_block->connect(_packed_to_unpacked,0,_scrambler,0);
    _top_block->connect(_scrambler,0,_diff_encoder,0);
    _top_block->connect(_diff_encoder,0,_ccsds_encoder,0);
    //_top_block->connect(_unpacked_to_packed,0,_ccsds_encoder,0);
    //_top_block->connect(_ccsds_encoder,0,_packed_to_unpacked2,0);
    _top_block->connect(_ccsds_encoder,0,_chunks_to_symbols,0);
    _top_block->connect(_chunks_to_symbols,0,_repeat,0);

    _top_block->connect(_repeat,0,_amplify,0);
    _top_block->connect(_amplify,0,_filter,0);

    _top_block->connect(_filter,0,_osmosdr_sink,0);

}




void gr_mod_bpsk_sdr::start()
{
    _top_block->start();
}

void gr_mod_bpsk_sdr::stop()
{
    _top_block->stop();
    _top_block->wait();
}

int gr_mod_bpsk_sdr::setData(std::vector<u_int8_t> *data)
{
    return _vector_source->set_data(data);

}

void gr_mod_bpsk_sdr::tune(long center_freq)
{
    _device_frequency = center_freq;
    _osmosdr_sink->set_center_freq(_device_frequency);
}

void gr_mod_bpsk_sdr::set_power(int dbm)
{
    _osmosdr_sink->set_gain(dbm);
}
