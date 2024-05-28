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

#include "settings.h"

Settings::Settings(Logger *logger)
{
    _logger = logger;
    _config_file = setupConfig();

    /// not saved to config
    headless_mode = false;
    rx_inited = false;
    tx_inited = false;
    tx_started = false;
    voip_connected = false;
    voip_forwarding = false;
    voip_ptt_enabled = false;
    udp_enabled = false;
    vox_enabled = false;
    repeater_enabled = false;
    current_voip_channel = -1;
    rssi = 0.0;

    /// saved to config
    demod_offset = 0;
    rx_mode = 0;
    tx_mode = 0;
    rx_ctcss = 0.0;
    tx_ctcss = 0.0;
    ip_address = "";
    rx_sample_rate = 1000000;
    scan_step = 0;
    show_controls = 0;
    show_constellation = 0;
    show_time_domain = 0;
    show_fft = 0;
    time_domain_sample_rate = 100000;
    time_domain_sample_scaling = 1000;
    time_domain_sample_speed = 50000;
    enable_duplex = 0;
    fft_size = 32768;
    waterfall_fps = 15;
    control_port = 4939;
    udp_listen_port = 4938;
    udp_send_port = 4937;
    voip_server="127.0.0.1";
    bb_gain = 1;
    night_mode = 0;
    tx_band_limits = 0;
    lime_rfe_device = "/dev/ttyUSB0";
    enable_lime_rfe = 0;
    lime_rfe_attenuation = 0;
    lime_rfe_notch = 0;
    mmdvm_channels = 3;
    mmdvm_channel_separation = 25000;
    tx_carrier_offset = 0;
    burst_delay_msec = 60;
    m17_can_tx = 0;
    m17_can_rx = 0;
    m17_dest = "ALL";
    m17_src = "";
    m17_decode_all_can = 1;
    m17_destination_type = 0;
    udp_audio_sample_rate = 48000;
    sql_pty_path = "/tmp/sql_pty";
    udp_audio_local_address = "127.0.0.1";
    udp_audio_remote_address = "127.0.0.1";

    /// old stuff, not used
    _mumble_tcp = 1; // used
    _use_codec2 = 0; // used
    _audio_treshhold = -15; // not used
    _voice_activation = 0.5; // not used
    _voice_activation_timeout = 50; // not used
    _ident_time = 300; // not used
    _radio_id = "";
    _id = 0;
}

Settings::~Settings()
{
    delete _config_file;
}

QFileInfo *Settings::setupConfig()
{
    QDir files = QDir::homePath();
    if(!QDir(files.absolutePath()+"/.config/qradiolink").exists())
    {
        QDir().mkdir(files.absolutePath()+"/.config/qradiolink");
    }
    QFileInfo old_file = files.filePath(".config/qradiolink.cfg");
    if(old_file.exists())
    {
        QDir().rename(old_file.filePath(), files.filePath(".config/qradiolink/qradiolink.cfg"));
    }
    QFileInfo new_file = files.filePath(".config/qradiolink/qradiolink.cfg");
    if(!new_file.exists())
    {
        QString config = "// Automatically generated\n";
        QFile newfile(new_file.absoluteFilePath());

        if (newfile.open(QIODevice::ReadWrite))
        {
            newfile.write(config.toStdString().c_str());
            newfile.close();
        }

    }

    return new QFileInfo(new_file);
}


void Settings::readConfig()
{
    libconfig::Config cfg;
    try
    {
        cfg.readFile(_config_file->absoluteFilePath().toStdString().c_str());
    }
    catch(const libconfig::FileIOException &fioex)
    {
        _logger->log(Logger::LogLevelFatal, "I/O error while reading configuration file.");
        exit(EXIT_FAILURE); // a bit radical
    }
    catch(const libconfig::ParseException &pex)
    {
        _logger->log(Logger::LogLevelFatal,
                  QString("Configuration parse error at %1: %2 - %3").arg(pex.getFile()).arg(
                         pex.getLine()).arg(pex.getError()));
        exit(EXIT_FAILURE); // a bit radical
    }

    /// Read values
    try
    {
        rx_freq_corr = cfg.lookup("rx_freq_corr");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_freq_corr = 0;
    }
    try
    {
        tx_freq_corr = cfg.lookup("tx_freq_corr");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_freq_corr = 0;
    }
    try
    {
        rx_device_args = QString(cfg.lookup("rx_device_args"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_device_args = "rtl=0";
    }
    try
    {
        tx_device_args = QString(cfg.lookup("tx_device_args"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_device_args = "soapy=0,driver=plutosdr";
    }
    try
    {
        rx_antenna = QString(cfg.lookup("rx_antenna"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_antenna = "Auto";
    }
    try
    {
        tx_antenna = QString(cfg.lookup("tx_antenna"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_antenna = "Auto";
    }
    try
    {
        callsign = QString(cfg.lookup("callsign"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        callsign = "CALL";
    }
    try
    {
        video_device = QString(cfg.lookup("video_device"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        video_device = "/dev/video0";
    }
    try
    {
        audio_input_device = QString(cfg.lookup("audio_input_device"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        audio_input_device = "default";
    }
    try
    {
        audio_output_device = QString(cfg.lookup("audio_output_device"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        audio_output_device = "default";
    }
    try
    {
        tx_power = cfg.lookup("tx_power");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_power = 50;
    }
    try
    {
        bb_gain = cfg.lookup("bb_gain");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        bb_gain = 5;
    }
    try
    {
        if_gain = cfg.lookup("if_gain");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        if_gain = 50;
    }
    try
    {
        agc_attack = cfg.lookup("agc_attack");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        agc_attack = 1;
    }
    try
    {
        agc_decay = cfg.lookup("agc_decay");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        agc_decay = 1;
    }
    try
    {
        rx_sensitivity = cfg.lookup("rx_sensitivity");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_sensitivity = 90;
    }
    try
    {
        squelch = cfg.lookup("squelch");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        squelch = -70;
    }
    try
    {
        rx_volume = cfg.lookup("rx_volume");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_volume = 30;
    }
    try
    {
        tx_volume = cfg.lookup("tx_volume");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_volume = 45;
    }
    try
    {
        voip_volume = cfg.lookup("voip_volume");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        voip_volume = 60;
    }
    try
    {
        rx_ctcss = cfg.lookup("rx_ctcss");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_ctcss = 0.0;
    }
    try
    {
        tx_ctcss = cfg.lookup("tx_ctcss");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_ctcss = 0.0;
    }
    try
    {
        rx_frequency = cfg.lookup("rx_frequency");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_frequency = 434000000;
    }
    try
    {
        demod_offset = cfg.lookup("demod_offset");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        demod_offset = 0;
    }
    try
    {
        tx_shift = cfg.lookup("tx_shift");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_shift = 0;
    }
    try
    {
        voip_server = QString(cfg.lookup("voip_server"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        voip_server = "127.0.0.1";
    }
    try
    {
        voip_port = cfg.lookup("voip_port");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        voip_port = 64738;
    }
    try
    {
        voip_password = QString(cfg.lookup("voip_password"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        voip_password = "";
    }
    try
    {
        rx_mode = cfg.lookup("rx_mode");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_mode = 0;
    }
    try
    {
        tx_mode = cfg.lookup("tx_mode");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_mode = 0;
    }
    try
    {
        ip_address = QString(cfg.lookup("ip_address"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        ip_address = "10.0.0.1";
    }
    try
    {
        rx_sample_rate = cfg.lookup("rx_sample_rate");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_sample_rate = 1000000;
    }
    try
    {
        tx_carrier_offset = cfg.lookup("tx_carrier_offset");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_carrier_offset = 0;
    }
    try
    {
        scan_step = cfg.lookup("scan_step");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        scan_step = 2500;
    }
    try
    {
        show_controls = cfg.lookup("show_controls");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        show_controls = 1;
    }
    try
    {
        show_constellation = cfg.lookup("show_constellation");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        show_constellation = 0;
    }
    try
    {
        show_time_domain = cfg.lookup("show_time_domain");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        show_time_domain = 0;
    }
    try
    {
        show_fft = cfg.lookup("show_fft");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        show_fft = 1;
    }
    try
    {
        fft_history = cfg.lookup("fft_history");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        fft_history = 1;
    }
    try
    {
        coloured_fft = cfg.lookup("coloured_fft");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        coloured_fft = 0;
    }
    try
    {
        fft_size = cfg.lookup("fft_size");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        fft_size = 32768;
    }
    try
    {
        fft_averaging = cfg.lookup("fft_averaging");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        fft_averaging = 1.0;
    }
    try
    {
        wf_averaging = cfg.lookup("wf_averaging");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        wf_averaging = 0;
    }
    try
    {
        draw_constellation_eye = cfg.lookup("draw_constellation_eye");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        draw_constellation_eye = 0;
    }
    try
    {
        waterfall_fps = cfg.lookup("waterfall_fps");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        waterfall_fps = 15;
    }
    try
    {
        enable_duplex = cfg.lookup("enable_duplex");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        enable_duplex = 0;
    }
    try
    {
        audio_compressor = cfg.lookup("audio_compressor");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        audio_compressor = 0;
    }
    try
    {
        enable_relays = cfg.lookup("enable_relays");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        enable_relays = 0;
    }
    try
    {
        rssi_calibration_value = cfg.lookup("rssi_calibration_value");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rssi_calibration_value = -80;
    }
    try
    {
        control_port = cfg.lookup("control_port");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        control_port = 4939;
    }
    try
    {
        udp_listen_port = cfg.lookup("udp_listen_port");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        udp_listen_port = 4938;
    }
    try
    {
        udp_send_port = cfg.lookup("udp_send_port");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        udp_send_port = 4937;
    }
    try
    {
        remote_control = cfg.lookup("remote_control");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        remote_control = 0;
    }
    try
    {
        burst_ip_modem = cfg.lookup("burst_ip_modem");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        burst_ip_modem = 0;
    }
    try
    {
        night_mode = cfg.lookup("night_mode");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        night_mode = 0;
    }
    try
    {
        scan_resume_time = cfg.lookup("scan_resume_time");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        scan_resume_time = 5;
    }
    try
    {
        audio_record_path = QString(cfg.lookup("audio_record_path"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        audio_record_path = QDir::homePath();
    }
    try
    {
        vox_level = cfg.lookup("vox_level");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        vox_level = 20;
    }
    try
    {
        voip_bitrate = cfg.lookup("voip_bitrate");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        voip_bitrate = 24600;
    }
    try
    {
        end_beep = cfg.lookup("end_beep");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        end_beep = 0;
    }
    try
    {
        mute_forwarded_audio = cfg.lookup("mute_forwarded_audio");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        mute_forwarded_audio = 1;
    }
    try
    {
        block_buffer_size = cfg.lookup("block_buffer_size");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        block_buffer_size = 0;
    }
    try
    {
        radio_tot = cfg.lookup("radio_tot");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        radio_tot = 120;
    }
    try
    {
        tot_tx_end = cfg.lookup("tot_tx_end");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tot_tx_end = 0;
    }
    try
    {
        tx_band_limits = cfg.lookup("tx_band_limits");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_band_limits = 1;
    }
    try
    {
        window_width = cfg.lookup("window_width");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        window_width = 1400;
    }
    try
    {
        window_height = cfg.lookup("window_height");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        window_height = 700;
    }
    try
    {
        relay_sequence = cfg.lookup("relay_sequence");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        relay_sequence = 15;
    }
    try
    {
        lnb_lo_freq = cfg.lookup("lnb_lo_freq");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        lnb_lo_freq = 0;
    }
    try
    {
        panadapter_min_db = cfg.lookup("panadapter_min_db");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        panadapter_min_db = -120.0;
    }
    try
    {
        panadapter_max_db = cfg.lookup("panadapter_max_db");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        panadapter_max_db = -30.0;
    }
    try
    {
        gpredict_control = cfg.lookup("gpredict_control");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        gpredict_control = 0;
    }
    try
    {
        lime_rfe_device = QString(cfg.lookup("lime_rfe_device"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        lime_rfe_device = "/dev/ttyUSB0";
    }
    try
    {
        enable_lime_rfe = cfg.lookup("enable_lime_rfe");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        enable_lime_rfe = 0;
    }
    try
    {
        lime_rfe_attenuation = cfg.lookup("lime_rfe_attenuation");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        lime_rfe_attenuation = 2;
    }
    try
    {
        lime_rfe_notch = cfg.lookup("lime_rfe_notch");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        lime_rfe_notch = 2;
    }
    try
    {
        mmdvm_channels = cfg.lookup("mmdvm_channels");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        mmdvm_channels = 3;
    }
    try
    {
        mmdvm_channel_separation = cfg.lookup("mmdvm_channel_separation");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        mmdvm_channel_separation = 25000;
    }
    try
    {
        burst_delay_msec = cfg.lookup("burst_delay_msec");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        burst_delay_msec = 60;
    }
    try
    {
        m17_can_tx = cfg.lookup("m17_can_tx");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        m17_can_tx = 0;
    }
    try
    {
        m17_can_rx = cfg.lookup("m17_can_rx");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        m17_can_rx = 0;
    }
    try
    {
        m17_src = QString(cfg.lookup("m17_src"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        m17_src = callsign;
    }
    try
    {
        m17_dest = QString(cfg.lookup("m17_dest"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        m17_dest = "";
    }
    try
    {
        m17_decode_all_can = cfg.lookup("m17_decode_all_can");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        m17_decode_all_can = 1;
    }
    try
    {
        m17_destination_type = cfg.lookup("m17_destination_type");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        m17_destination_type = 0;
    }
    try
    {
        udp_audio_sample_rate = cfg.lookup("udp_audio_sample_rate");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        udp_audio_sample_rate = 48000;
    }
    try
    {
        sql_pty_path = QString(cfg.lookup("sql_pty_path"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        sql_pty_path = "/tmp/sql_pty";
    }
    try
    {
        udp_audio_local_address = QString(cfg.lookup("udp_audio_local_address"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        udp_audio_local_address = "127.0.0.1";
    }
    try
    {
        udp_audio_remote_address = QString(cfg.lookup("udp_audio_remote_address"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        udp_audio_remote_address = "127.0.0.1";
    }
    try
    {
        time_domain_sample_rate = cfg.lookup("time_domain_sample_rate");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        time_domain_sample_rate = 100000;
    }
    try
    {
        time_domain_sample_scaling = cfg.lookup("time_domain_sample_scaling");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        time_domain_sample_scaling = 1000;
    }
    try
    {
        time_domain_sample_speed = cfg.lookup("time_domain_sample_speed");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        time_domain_sample_speed = 50000;
    }


}

void Settings::saveConfig()
{
    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();
    root.add("rx_device_args",libconfig::Setting::TypeString) = rx_device_args.toStdString();
    root.add("tx_device_args",libconfig::Setting::TypeString) = tx_device_args.toStdString();
    root.add("rx_antenna",libconfig::Setting::TypeString) = rx_antenna.toStdString();
    root.add("tx_antenna",libconfig::Setting::TypeString) = tx_antenna.toStdString();
    root.add("rx_freq_corr",libconfig::Setting::TypeInt) = (int)rx_freq_corr;
    root.add("tx_freq_corr",libconfig::Setting::TypeInt) = (int)tx_freq_corr;
    root.add("callsign",libconfig::Setting::TypeString) = callsign.toStdString();
    root.add("video_device",libconfig::Setting::TypeString) = video_device.toStdString();
    root.add("audio_input_device",libconfig::Setting::TypeString) = audio_input_device.toStdString();
    root.add("audio_output_device",libconfig::Setting::TypeString) = audio_output_device.toStdString();
    root.add("tx_power",libconfig::Setting::TypeInt) = tx_power;
    root.add("bb_gain",libconfig::Setting::TypeInt) = bb_gain;
    root.add("if_gain",libconfig::Setting::TypeInt) = if_gain;
    root.add("rx_sensitivity",libconfig::Setting::TypeInt) = rx_sensitivity;
    root.add("squelch",libconfig::Setting::TypeInt) = squelch;
    root.add("rx_volume",libconfig::Setting::TypeInt) = rx_volume;
    root.add("tx_volume",libconfig::Setting::TypeInt) = tx_volume;
    root.add("voip_volume",libconfig::Setting::TypeInt) = voip_volume;
    root.add("rx_frequency",libconfig::Setting::TypeInt64) = rx_frequency;
    root.add("tx_carrier_offset",libconfig::Setting::TypeInt64) = tx_carrier_offset;
    root.add("tx_shift",libconfig::Setting::TypeInt64) = tx_shift;
    root.add("voip_server",libconfig::Setting::TypeString) = voip_server.toStdString();
    root.add("voip_port",libconfig::Setting::TypeInt) = voip_port;
    root.add("voip_password",libconfig::Setting::TypeString) = voip_password.toStdString();
    root.add("rx_mode",libconfig::Setting::TypeInt) = rx_mode;
    root.add("tx_mode",libconfig::Setting::TypeInt) = tx_mode;
    root.add("rx_ctcss",libconfig::Setting::TypeFloat) = rx_ctcss;
    root.add("tx_ctcss",libconfig::Setting::TypeFloat) = tx_ctcss;
    root.add("ip_address",libconfig::Setting::TypeString) = ip_address.toStdString();
    root.add("demod_offset",libconfig::Setting::TypeInt64) = demod_offset;
    root.add("rx_sample_rate",libconfig::Setting::TypeInt64) = rx_sample_rate;
    root.add("scan_step",libconfig::Setting::TypeInt) = scan_step;
    root.add("show_controls",libconfig::Setting::TypeInt) = show_controls;
    root.add("show_constellation",libconfig::Setting::TypeInt) = show_constellation;
    root.add("show_time_domain",libconfig::Setting::TypeInt) = show_time_domain;
    root.add("show_fft",libconfig::Setting::TypeInt) = show_fft;
    root.add("fft_history",libconfig::Setting::TypeInt) = fft_history;
    root.add("coloured_fft",libconfig::Setting::TypeInt) = coloured_fft;
    root.add("enable_duplex",libconfig::Setting::TypeInt) = enable_duplex;
    root.add("fft_size",libconfig::Setting::TypeInt) = fft_size;
    root.add("fft_averaging",libconfig::Setting::TypeFloat) = fft_averaging;
    root.add("wf_averaging",libconfig::Setting::TypeInt) = wf_averaging;
    root.add("draw_constellation_eye",libconfig::Setting::TypeInt) = draw_constellation_eye;
    root.add("waterfall_fps",libconfig::Setting::TypeInt) = waterfall_fps;
    root.add("audio_compressor",libconfig::Setting::TypeInt) = audio_compressor;
    root.add("enable_relays",libconfig::Setting::TypeInt) = enable_relays;
    root.add("rssi_calibration_value",libconfig::Setting::TypeInt) = rssi_calibration_value;
    root.add("control_port",libconfig::Setting::TypeInt) = control_port;
    root.add("udp_listen_port",libconfig::Setting::TypeInt) = udp_listen_port;
    root.add("udp_send_port",libconfig::Setting::TypeInt) = udp_send_port;
    root.add("agc_attack",libconfig::Setting::TypeInt) = agc_attack;
    root.add("agc_decay",libconfig::Setting::TypeInt) = agc_decay;
    root.add("remote_control",libconfig::Setting::TypeInt) = remote_control;
    root.add("burst_ip_modem",libconfig::Setting::TypeInt) = burst_ip_modem;
    root.add("night_mode",libconfig::Setting::TypeInt) = night_mode;
    root.add("scan_resume_time",libconfig::Setting::TypeInt) = scan_resume_time;
    root.add("audio_record_path",libconfig::Setting::TypeString) = audio_record_path.toStdString();
    root.add("vox_level",libconfig::Setting::TypeInt) = vox_level;
    root.add("voip_bitrate",libconfig::Setting::TypeInt) = voip_bitrate;
    root.add("end_beep",libconfig::Setting::TypeInt) = end_beep;
    root.add("mute_forwarded_audio",libconfig::Setting::TypeInt) = mute_forwarded_audio;
    root.add("block_buffer_size",libconfig::Setting::TypeInt) = block_buffer_size;
    root.add("radio_tot",libconfig::Setting::TypeInt) = radio_tot;
    root.add("tot_tx_end",libconfig::Setting::TypeInt) = tot_tx_end;
    root.add("tx_band_limits",libconfig::Setting::TypeInt) = tx_band_limits;
    root.add("window_width",libconfig::Setting::TypeInt) = window_width;
    root.add("window_height",libconfig::Setting::TypeInt) = window_height;
    root.add("relay_sequence",libconfig::Setting::TypeInt) = relay_sequence;
    root.add("lnb_lo_freq",libconfig::Setting::TypeInt64) = lnb_lo_freq;
    root.add("panadapter_min_db",libconfig::Setting::TypeFloat) = panadapter_min_db;
    root.add("panadapter_max_db",libconfig::Setting::TypeFloat) = panadapter_max_db;
    root.add("gpredict_control",libconfig::Setting::TypeInt) = gpredict_control;
    root.add("lime_rfe_device",libconfig::Setting::TypeString) = lime_rfe_device.toStdString();
    root.add("enable_lime_rfe",libconfig::Setting::TypeInt) = enable_lime_rfe;
    root.add("lime_rfe_attenuation",libconfig::Setting::TypeInt) = lime_rfe_attenuation;
    root.add("lime_rfe_notch",libconfig::Setting::TypeInt) = lime_rfe_notch;
    root.add("mmdvm_channels",libconfig::Setting::TypeInt) = mmdvm_channels;
    root.add("mmdvm_channel_separation",libconfig::Setting::TypeInt) = mmdvm_channel_separation;
    root.add("burst_delay_msec",libconfig::Setting::TypeInt) = burst_delay_msec;
    root.add("m17_can_tx",libconfig::Setting::TypeInt) = m17_can_tx;
    root.add("m17_can_rx",libconfig::Setting::TypeInt) = m17_can_rx;
    root.add("m17_src",libconfig::Setting::TypeString) = m17_src.toStdString();
    root.add("m17_dest",libconfig::Setting::TypeString) = m17_dest.toStdString();
    root.add("m17_decode_all_can",libconfig::Setting::TypeInt) = m17_decode_all_can;
    root.add("m17_destination_type",libconfig::Setting::TypeInt) = m17_destination_type;
    root.add("udp_audio_sample_rate",libconfig::Setting::TypeInt) = udp_audio_sample_rate;
    root.add("sql_pty_path",libconfig::Setting::TypeString) = sql_pty_path.toStdString();
    root.add("udp_audio_local_address",libconfig::Setting::TypeString) = udp_audio_local_address.toStdString();
    root.add("udp_audio_remote_address",libconfig::Setting::TypeString) = udp_audio_remote_address.toStdString();
    root.add("time_domain_sample_rate",libconfig::Setting::TypeInt) = time_domain_sample_rate;
    root.add("time_domain_sample_scaling",libconfig::Setting::TypeInt) = time_domain_sample_scaling;
    root.add("time_domain_sample_speed",libconfig::Setting::TypeInt) = time_domain_sample_speed;
    try
    {
        cfg.writeFile(_config_file->absoluteFilePath().toStdString().c_str());
    }
    catch(const libconfig::FileIOException &fioex)
    {
        _logger->log(Logger::LogLevelFatal, "I/O error while writing configuration file: " +
                     _config_file->absoluteFilePath());
        exit(EXIT_FAILURE);
    }
}

