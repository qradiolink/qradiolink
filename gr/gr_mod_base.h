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

#ifndef GR_MOD_BASE_H
#define GR_MOD_BASE_H

#include <QObject>
#include "modem_types.h"
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/rotator_cc.h>
#include <gnuradio/vocoder/freedv_api.h>
#include <gnuradio/constants.h>
#include <osmosdr/sink.h>
#include <vector>
#include "gr_vector_source.h"
#include "gr_audio_source.h"
#include "gr_mod_2fsk_sdr.h"
#include "gr_mod_4fsk_sdr.h"
#include "gr_mod_am_sdr.h"
#include "gr_mod_bpsk_sdr.h"
#include "gr_mod_nbfm_sdr.h"
#include "gr_mod_qpsk_sdr.h"
#include "gr_mod_ssb_sdr.h"
#include "gr_mod_freedv.h"

class gr_mod_base : public QObject
{
    Q_OBJECT
public:
    explicit gr_mod_base(QObject *parent = 0, float device_frequency=434000000,
                float rf_gain=0.5, std::string device_args="uhd", std::string device_antenna="TX/RX", int freq_corr=0);

public slots:
    void start();
    void stop();
    int set_data(std::vector<u_int8_t> *data);
    void tune(long center_freq);
    void set_power(float dbm);
    void set_ctcss(float value);
    void set_mode(int mode);
    int set_audio(std::vector<float> *data);
    void set_bb_gain(int value);
    void set_carrier_offset(long carrier_offset);
    void flush_sources();

private:
    gr::top_block_sptr _top_block;
    gr_vector_source_sptr _vector_source;
    gr_audio_source_sptr _audio_source;
    osmosdr::sink::sptr _osmosdr_sink;
    gr::blocks::rotator_cc::sptr _rotator;

    gr_mod_2fsk_sdr_sptr _2fsk;
    gr_mod_2fsk_sdr_sptr _2fsk_10k;
    gr_mod_4fsk_sdr_sptr _4fsk_2k;
    gr_mod_4fsk_sdr_sptr _4fsk_10k;
    gr_mod_am_sdr_sptr _am;
    gr_mod_bpsk_sdr_sptr _bpsk_1k;
    gr_mod_bpsk_sdr_sptr _bpsk_2k;
    gr_mod_nbfm_sdr_sptr _fm_2500;
    gr_mod_nbfm_sdr_sptr _fm_5000;
    gr_mod_qpsk_sdr_sptr _qpsk_2k;
    gr_mod_qpsk_sdr_sptr _qpsk_10k;
    gr_mod_qpsk_sdr_sptr _qpsk_250k;
    gr_mod_qpsk_sdr_sptr _qpsk_video;
    gr_mod_ssb_sdr_sptr _usb;
    gr_mod_ssb_sdr_sptr _lsb;
    gr_mod_freedv_sdr_sptr _freedv_tx1600_usb;
    gr_mod_freedv_sdr_sptr _freedv_tx700C_usb;
    gr_mod_freedv_sdr_sptr _freedv_tx1600_lsb;
    gr_mod_freedv_sdr_sptr _freedv_tx700C_lsb;
    gr_mod_freedv_sdr_sptr _freedv_tx800XA_usb;
    gr_mod_freedv_sdr_sptr _freedv_tx800XA_lsb;

    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    float _device_frequency;
    int _carrier_offset;
    int _mode;
    bool _lime_specific; // FIXME: ugly hack
    double _osmo_filter_bw;
    void set_bandwidth_specific();
    osmosdr::gain_range_t _gain_range;


};

#endif // GR_MOD_BASE_H
