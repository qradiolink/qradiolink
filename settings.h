// Written by Adrian Musceac YO8RZZ , started August 2016.
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <libconfig.h++>
#include <iostream>

class Settings
{
public:
    Settings();
    QFileInfo *setupConfig();
    void readConfig();
    void saveConfig();

    /// Saved to config file
    QString rx_device_args;
    QString tx_device_args;
    QString rx_antenna;
    QString tx_antenna;
    int tx_power;
    int bb_gain;
    int rx_sensitivity;
    int rx_freq_corr;
    int tx_freq_corr;
    int squelch;
    int rx_volume;
    int tx_volume;
    float rx_ctcss;
    float tx_ctcss;
    long long rx_frequency;
    long long tx_shift;
    QString callsign;
    QString video_device;
    QString voip_server;
    int voip_port;
    int rx_mode;
    int tx_mode;
    QString ip_address;
    long long demod_offset;
    long long rx_sample_rate;
    int scan_step;
    int show_controls;
    int show_constellation;
    int enable_duplex;
    int fft_size;
    float fft_averaging;
    int waterfall_fps;
    int show_fft;
    int audio_compressor;
    int enable_relays;
    int rssi_calibration_value;
    QString audio_output_device;
    QString audio_input_device;
    int control_port; // FIXME: this should be unsigned uint16
    int agc_attack;
    int agc_decay;

    /// Used by remote interface
    bool _rx_status;
    bool _tx_status;
    int _rssi;
     // FIXME: this should be set by radioop

    /// Used by both GUI and remote interface
    bool _in_transmission;
    qint64 _tx_frequency;
    bool _voip_connected;
    bool _vox_enabled;
    bool _repeater_enabled;
    bool _voip_forwarding;
    int _current_voip_channel;


    /// Old stuff, not used now
    quint32 _id;
    quint8 _mumble_tcp;
    quint8 _use_codec2;
    quint8 _use_dtmf;
    float _audio_treshhold;
    float _voice_activation;
    quint16 _voice_activation_timeout;
    quint16 _local_udp_port;
    quint8 _enable_vox;
    quint8 _enable_agc;
    quint16 _ident_time;
    QString _radio_id;


private:
    QFileInfo *_config_file;
};

#endif // SETTINGS_H
