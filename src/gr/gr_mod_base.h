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
#include <QMap>
#include <QVector>
#include <QDebug>
#include <string>
#include <vector>
#include <gnuradio/vocoder/freedv_api.h>
#undef I // FIXME: remove this workaround later
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/rotator_cc.h>
#include <gnuradio/constants.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/copy.h>
#include <gnuradio/zeromq/pull_source.h>
#include <gnuradio/filter/rational_resampler_base.h>
#include <osmosdr/sink.h>
#include "src/modem_types.h"
#include "gr_byte_source.h"
#include "gr_audio_source.h"
#include "gr_mod_2fsk.h"
#include "gr_mod_gmsk.h"
#include "gr_mod_4fsk.h"
#include "gr_mod_am.h"
#include "gr_mod_bpsk.h"
#include "gr_mod_nbfm.h"
#include "gr_mod_qpsk.h"
#include "gr_mod_ssb.h"
#include "gr_mod_freedv.h"
#include "gr_mod_dsss.h"
#include "gr_mod_mmdvm.h"


class gr_mod_base : public QObject
{
    Q_OBJECT
public:
    explicit gr_mod_base(QObject *parent = 0, float device_frequency=434000000,
                float rf_gain=0.5, std::string device_args="uhd", std::string device_antenna="TX/RX", int freq_corr=0);

public slots:
    void start(int buffer_size=0);
    void stop();
    int set_data(std::vector<u_int8_t> *data);
    void tune(int64_t center_freq);
    void set_power(float value, std::string gain_stage="");
    void set_filter_width(int filter_width, int mode);
    void set_ctcss(float value);
    void set_mode(int mode);
    int set_audio(std::vector<float> *data);
    void set_bb_gain(float value);
    void set_cw_k(bool value);
    void set_carrier_offset(int64_t carrier_offset);
    int64_t reset_carrier_offset();
    void flush_sources();
    const QMap<std::string,QVector<int>> get_gain_names() const;
    void set_samp_rate(int samp_rate);

private:
    gr::top_block_sptr _top_block;
    gr_byte_source_sptr _byte_source;
    gr_audio_source_sptr _audio_source;
    osmosdr::sink::sptr _osmosdr_sink;
    gr::blocks::rotator_cc::sptr _rotator;
    gr::analog::sig_source_f::sptr _signal_source;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::zeromq::pull_source::sptr _zmq_source;

    gr_mod_2fsk_sptr _2fsk_2k_fm;
    gr_mod_2fsk_sptr _2fsk_1k_fm;
    gr_mod_2fsk_sptr _2fsk_2k;
    gr_mod_2fsk_sptr _2fsk_1k;
    gr_mod_2fsk_sptr _2fsk_10k;
    gr_mod_gmsk_sptr _gmsk_2k;
    gr_mod_gmsk_sptr _gmsk_1k;
    gr_mod_gmsk_sptr _gmsk_10k;
    gr_mod_4fsk_sptr _4fsk_2k;
    gr_mod_4fsk_sptr _4fsk_1k;
    gr_mod_4fsk_sptr _4fsk_2k_fm;
    gr_mod_4fsk_sptr _4fsk_1k_fm;
    gr_mod_4fsk_sptr _4fsk_10k_fm;
    gr_mod_am_sptr _am;
    gr_mod_bpsk_sptr _bpsk_1k;
    gr_mod_bpsk_sptr _bpsk_2k;
    gr_mod_dsss_sptr _bpsk_dsss_8;
    gr_mod_nbfm_sptr _fm_2500;
    gr_mod_nbfm_sptr _fm_5000;
    gr_mod_qpsk_sptr _qpsk_2k;
    gr_mod_qpsk_sptr _qpsk_10k;
    gr_mod_qpsk_sptr _qpsk_250k;
    gr_mod_qpsk_sptr _qpsk_video;
    gr_mod_4fsk_sptr _4fsk_96k;
    gr_mod_ssb_sptr _usb;
    gr_mod_ssb_sptr _lsb;
    gr_mod_ssb_sptr _usb_cw;
    gr_mod_freedv_sptr _freedv_tx1600_usb;
    gr_mod_freedv_sptr _freedv_tx700C_usb;
    gr_mod_freedv_sptr _freedv_tx700D_usb;
    gr_mod_freedv_sptr _freedv_tx1600_lsb;
    gr_mod_freedv_sptr _freedv_tx700C_lsb;
    gr_mod_freedv_sptr _freedv_tx700D_lsb;
    gr_mod_freedv_sptr _freedv_tx800XA_usb;
    gr_mod_freedv_sptr _freedv_tx800XA_lsb;
    gr_mod_mmdvm_sptr _mmdvm_mod;


    int _samples_per_symbol;
    int _samp_rate;
    int _carrier_freq;
    int _filter_width;
    double _device_frequency;
    int _freq_correction;
    int _carrier_offset;
    int _preserve_carrier_offset;
    int _mode;
    bool _lime_specific; // FIXME: ugly hack
    double _osmo_filter_bw;
    void set_bandwidth_specific();
    osmosdr::gain_range_t _gain_range;
    std::vector<std::string> _gain_names;

};

#endif // GR_MOD_BASE_H
