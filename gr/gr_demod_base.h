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
#include <gnuradio/top_block.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/qtgui/const_sink_c.h>
#include <gnuradio/qtgui/sink_c.h>
#include <gnuradio/qtgui/number_sink.h>
#include <gnuradio/analog/agc2_ff.h>
#include <gnuradio/blocks/complex_to_mag_squared.h>
#include <gnuradio/blocks/nlog10_ff.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/single_pole_iir_filter_ff.h>
#include <gnuradio/blocks/moving_average_ff.h>
#include <gnuradio/blocks/add_const_ff.h>
#include <gnuradio/blocks/delay.h>
#include <gnuradio/blocks/copy.h>
#include <gnuradio/blocks/message_debug.h>
#include <osmosdr/source.h>
#include <vector>
#include "gr_audio_sink.h"
#include "gr_vector_sink.h"
#include "gr_demod_2fsk_sdr.h"
#include "gr_demod_4fsk_sdr.h"
#include "gr_demod_am_sdr.h"
#include "gr_demod_bpsk_sdr.h"
#include "gr_demod_nbfm_sdr.h"
#include "gr_demod_qpsk_sdr.h"
#include "gr_demod_ssb_sdr.h"
#include "gr_demod_wbfm_sdr.h"
#include "modem_types.h"

class gr_demod_base : public QObject
{
    Q_OBJECT
public:
    explicit gr_demod_base(gr::qtgui::sink_c::sptr fft_gui,
                               gr::qtgui::const_sink_c::sptr const_gui, gr::qtgui::number_sink::sptr rssi_gui,
                                QObject *parent = 0, float device_frequency=434000000,
                               float rf_gain=50, std::string device_args="rtl=0", std::string device_antenna="RX2",
                                int freq_corr=0);
    ~gr_demod_base();
signals:

public slots:
    void start();
    void stop();
    std::vector<unsigned char> *getData();
    std::vector<unsigned char> *getFrame1();
    std::vector<unsigned char> *getFrame2();
    std::vector<float> *getAudio();
    void tune(long center_freq);
    void set_rx_sensitivity(float value);
    void set_squelch(int value);
    void set_ctcss(float value);
    void enable_gui_const(bool value);
    void enable_gui_fft(bool value);
    double get_freq();
    void set_mode(int mode);

private:
    gr::top_block_sptr _top_block;
    gr_audio_sink_sptr _audio_sink;
    gr_vector_sink_sptr _vector_sink;
    gr::analog::agc2_ff::sptr _agc2;
    gr::qtgui::const_sink_c::sptr _constellation;
    gr::qtgui::sink_c::sptr _fft_gui;
    gr::qtgui::number_sink::sptr _rssi;
    gr::blocks::message_debug::sptr _message_sink;
    gr::blocks::copy::sptr _rssi_valve;
    gr::blocks::copy::sptr _fft_valve;
    gr::blocks::copy::sptr _const_valve;
    gr::blocks::complex_to_mag_squared::sptr _mag_squared;
    gr::blocks::nlog10_ff::sptr _log10;
    gr::filter::single_pole_iir_filter_ff::sptr _single_pole_filter;
    gr::blocks::multiply_const_ff::sptr _multiply_const_ff;
    gr::blocks::moving_average_ff::sptr _moving_average;
    gr::blocks::add_const_ff::sptr _add_const;

    gr::analog::sig_source_c::sptr _signal_source;
    gr::blocks::multiply_cc::sptr _multiply;



    gr_demod_2fsk_sdr_sptr _2fsk;
    gr_demod_4fsk_sdr_sptr _4fsk_2k;
    gr_demod_4fsk_sdr_sptr _4fsk_10k;
    gr_demod_am_sdr_sptr _am;
    gr_demod_bpsk_sdr_sptr _bpsk_1k;
    gr_demod_bpsk_sdr_sptr _bpsk_2k;
    gr_demod_nbfm_sdr_sptr _fm_2500;
    gr_demod_nbfm_sdr_sptr _fm_5000;
    gr_demod_qpsk_sdr_sptr _qpsk_2k;
    gr_demod_qpsk_sdr_sptr _qpsk_10k;
    gr_demod_qpsk_sdr_sptr _qpsk_250k;
    gr_demod_qpsk_sdr_sptr _qpsk_video;
    gr_demod_ssb_sdr_sptr _ssb;
    gr_demod_wbfm_sdr_sptr _wfm;

    osmosdr::source::sptr _osmosdr_source;

    float _device_frequency;
    int _msg_nr;
    int _mode;
};

#endif // GR_DEMOD_BASE_H
