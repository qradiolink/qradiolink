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

#ifndef GR_DEMOD_BASE_H
#define GR_DEMOD_BASE_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <string>
#include <gnuradio/top_block.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base.h>
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/analog/agc2_ff.h>
#include <gnuradio/blocks/delay.h>
#include <gnuradio/blocks/copy.h>
#include <gnuradio/blocks/rotator_cc.h>
#include <gnuradio/blocks/message_debug.h>
#include <gnuradio/blocks/probe_signal.h>
#include <gnuradio/zeromq/push_sink.h>
#include <gnuradio/constants.h>
#include <osmosdr/source.h>
#include "limesdr/source.h"
#include <vector>
#include "gr_audio_sink.h"
#include "gr_bit_sink.h"
#include "gr_const_sink.h"
#include "rx_fft.h"
#include "gr_deframer_bb.h"
#include "gr_demod_2fsk.h"
#include "gr_demod_gmsk.h"
#include "gr_demod_4fsk.h"
#include "gr_demod_am.h"
#include "gr_demod_bpsk.h"
#include "gr_demod_nbfm.h"
#include "gr_demod_qpsk.h"
#include "gr_demod_ssb.h"
#include "gr_demod_wbfm.h"
#include "gr_demod_freedv.h"
#include "gr_demod_dsss.h"
#include "gr_demod_mmdvm.h"
#include "gr_mmdvm_sink.h"
#include "gr_demod_mmdvm_multi.h"
#include "gr_demod_m17.h"
#include "src/modem_types.h"
#include "src/bursttimer.h"
#include "rssi_block.h"

class gr_demod_base : public QObject
{
    Q_OBJECT
public:
    explicit gr_demod_base(BurstTimer *burst_timer, QObject *parent = 0, float device_frequency=434000000,
                               float rf_gain=50, std::string device_args="rtl=0", std::string device_antenna="RX2",
                                int freq_corr=0, int mmdvm_channels=3, int mmdvm_channel_separation=25000);
    ~gr_demod_base();

    void set_bandwidth_specific();

signals:

public slots:
    void start(int buffer_size=0);
    void stop();
    std::vector<unsigned char> *getData();
    std::vector<unsigned char> *getData(int nr);
    std::vector<float> *getAudio();
    void get_FFT_data(float *fft_data,  unsigned int &fftSize);
    void tune(int64_t center_freq);
    void set_carrier_offset(int64_t carrier_offset);
    void set_rx_sensitivity(double value, std::string gain_stage="");
    void set_squelch(int value);
    void set_gain(float value);
    void set_agc_attack(int value);
    void set_agc_decay(int value);
    void set_ctcss(float value);
    void enable_gui_const(bool value);
    void enable_gui_fft(bool value);
    void enable_rssi(bool value);
    void enable_demodulator(bool value);
    double get_freq();
    void set_mode(int mode, bool disconnect=true, bool connect=true);
    void set_fft_size(int size);
    float get_rssi();
    std::vector<gr_complex> *get_constellation_data();
    void set_samp_rate(int samp_rate);
    void set_filter_width(int filter_width, int mode);
    void calibrate_rssi(float value);
    const QMap<std::string,QVector<int>> get_gain_names() const;

private:
    gr::top_block_sptr _top_block;
    gr_audio_sink_sptr _audio_sink;
    gr_bit_sink_sptr _bit_sink;
    rx_fft_c_sptr _fft_sink;
    gr::blocks::message_debug::sptr _message_sink;
    gr::blocks::copy::sptr _rssi_valve;
    gr::blocks::copy::sptr _const_valve;
    gr::blocks::copy::sptr _demod_valve;
    gr::blocks::probe_signal_f::sptr _rssi;
    gr_const_sink_sptr _constellation;

    gr::blocks::rotator_cc::sptr _rotator;
    gr::filter::rational_resampler_base_ccf::sptr _resampler;
    gr::zeromq::push_sink::sptr _zeromq_sink;

    rssi_block_sptr _rssi_block;
    gr_deframer_bb_sptr _deframer1;
    gr_deframer_bb_sptr _deframer2;
    gr_deframer_bb_sptr _deframer_700_1;
    gr_deframer_bb_sptr _deframer_700_2;
    gr_deframer_bb_sptr _deframer1_10k;
    gr_deframer_bb_sptr _deframer2_10k;

    gr_demod_2fsk_sptr _2fsk_2k_fm;
    gr_demod_2fsk_sptr _2fsk_1k_fm;
    gr_demod_2fsk_sptr _2fsk_2k;
    gr_demod_2fsk_sptr _2fsk_1k;
    gr_demod_2fsk_sptr _2fsk_10k;
    gr_demod_gmsk_sptr _gmsk_2k;
    gr_demod_gmsk_sptr _gmsk_1k;
    gr_demod_gmsk_sptr _gmsk_10k;
    gr_demod_4fsk_sptr _4fsk_2k;
    gr_demod_4fsk_sptr _4fsk_2k_fm;
    gr_demod_4fsk_sptr _4fsk_1k_fm;
    gr_demod_4fsk_sptr _4fsk_10k_fm;
    gr_demod_am_sptr _am;
    gr_demod_bpsk_sptr _bpsk_1k;
    gr_demod_bpsk_sptr _bpsk_2k;
    gr_demod_dsss_sptr _bpsk_dsss_8;
    gr_demod_nbfm_sptr _fm_2500;
    gr_demod_nbfm_sptr _fm_5000;
    gr_demod_qpsk_sptr _qpsk_2k;
    gr_demod_qpsk_sptr _qpsk_10k;
    gr_demod_qpsk_sptr _qpsk_250k;
    gr_demod_qpsk_sptr _qpsk_video;
    gr_demod_4fsk_sptr _4fsk_96k;
    gr_demod_ssb_sptr _usb;
    gr_demod_ssb_sptr _lsb;
    gr_demod_wbfm_sptr _wfm;
    gr_demod_freedv_sptr _freedv_rx1600_usb;
    gr_demod_freedv_sptr _freedv_rx700C_usb;
    gr_demod_freedv_sptr _freedv_rx700D_usb;
    gr_demod_freedv_sptr _freedv_rx800XA_usb;
    gr_demod_freedv_sptr _freedv_rx1600_lsb;
    gr_demod_freedv_sptr _freedv_rx700C_lsb;
    gr_demod_freedv_sptr _freedv_rx700D_lsb;
    gr_demod_freedv_sptr _freedv_rx800XA_lsb;
    gr_demod_mmdvm_sptr _mmdvm_demod;
    gr_demod_mmdvm_multi_sptr _mmdvm_demod_multi;
    gr_mmdvm_sink_sptr _mmdvm_sink;
    gr_demod_m17_sptr _m17_demod;

    osmosdr::source::sptr _osmosdr_source;
    gr::limesdr::source::sptr _limesdr_source;

    float _device_frequency;
    int _freq_correction;
    int _mmdvm_channels;
    int _msg_nr;
    int _mode;
    int _carrier_offset;
    bool _demod_running;
    int _samp_rate;
    bool _locked;
    bool _lime_specific; // LimeSDR specific
    bool _use_tdma;
    double _osmo_filter_bw;
    osmosdr::gain_range_t _gain_range;
    std::vector<std::string> _gain_names;
};

#endif // GR_DEMOD_BASE_H
