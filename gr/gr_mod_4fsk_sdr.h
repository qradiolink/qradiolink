// Written by Adrian Musceac YO8RZZ , started March 2016.
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

#ifndef GR_MOD_4FSK_SDR_H
#define GR_MOD_4FSK_SDR_H

#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_source_b.h>
#include <gnuradio/blocks/packed_to_unpacked_bb.h>
#include <gnuradio/audio/sink.h>
#include <gnuradio/endianness.h>
#include <gnuradio/digital/chunks_to_symbols_bf.h>
#include <gnuradio/blocks/repeat.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/sig_source_waveform.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/multiply_const_cc.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/digital/diff_encoder_bb.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/digital/map_bb.h>
#include <gnuradio/digital/scrambler_bb.h>
#include <gnuradio/blocks/pack_k_bits_bb.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <osmosdr/sink.h>
#include <vector>
#include "gr_vector_source.h"
#include <QDebug>
#include <QObject>

class gr_mod_4fsk_sdr : public QObject
{
    Q_OBJECT
public:
    explicit gr_mod_4fsk_sdr(QObject *parent = 0, int sps=125, int samp_rate=250000, int carrier_freq=1700,
                             int filter_width=8000, float mod_index=1, float device_frequency=434025000,
                             float rf_gain=0.5, std::string device_args="uhd", std::string device_antenna="TX/RX", int freq_corr=0);
    ~gr_mod_4fsk_sdr();
public slots:
    void start();
    void stop();
    int setData(std::vector<u_int8_t> *data);
    void tune(long center_freq);
    void set_power(float dbm);

private:
    gr::top_block_sptr _top_block;
    gr_vector_source_sptr _vector_source;
    gr::blocks::packed_to_unpacked_bb::sptr _packed_to_unpacked;
    gr::digital::chunks_to_symbols_bf::sptr _chunks_to_symbols;
    gr::blocks::multiply_const_cc::sptr _amplify;
    gr::digital::scrambler_bb::sptr _scrambler;
    gr::blocks::repeat::sptr _repeat;
    gr::filter::fft_filter_ccf::sptr _filter;
    gr::digital::diff_encoder_bb::sptr _diff_encoder;
    gr::blocks::pack_k_bits_bb::sptr _packer;
    gr::digital::map_bb::sptr _map;
    gr::analog::frequency_modulator_fc::sptr _freq_modulator;
    osmosdr::sink::sptr _osmosdr_sink;


    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _modulation_index;
    float _device_frequency;

};


#endif // GR_MOD_4FSK_SDR_H
