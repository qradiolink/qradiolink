#include "settings.h"

Settings::Settings()
{
    _id = 0;

    _mumble_tcp = 1; // used
    _use_codec2 = 0; // used
    _use_dtmf = 0; // used
    _audio_treshhold = -15; // used
    _voice_activation = 0.5; // used
    _voice_activation_timeout = 50; // used
    _voice_server_port = 64738;
    _local_udp_port = 4938;
    _control_port = 4939;
    _enable_vox = 0; // unused
    _enable_agc = 0; // unused
    _ident_time = 300; // used
    _radio_id = "";
    demod_offset = 0;
    rx_mode = 0;
    tx_mode = 0;
    ip_address = "";
    rx_sample_rate = 1000000;
    scan_step = 0;
    show_controls = 0;
    show_constellation = 0;
    show_fft = 0;
    enable_duplex = 0;
    fft_size = 32768;
    waterfall_fps = 15;

    voip_server="127.0.0.1";
    _config_file = setupConfig();
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
        std::cerr << "I/O error while reading configuration file." << std::endl;
        exit(EXIT_FAILURE);
    }
    catch(const libconfig::ParseException &pex)
    {
        std::cerr << "Configuration parse error at " << pex.getFile() << ":" << pex.getLine()
                  << " - " << pex.getError() << std::endl;
        exit(EXIT_FAILURE);
    }

    /// Read values
    try
    {
        cfg.lookupValue("rx_freq_corr", rx_freq_corr);
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_freq_corr = 0;
    }
    try
    {
        cfg.lookupValue("tx_freq_corr", tx_freq_corr);
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
        tx_device_args = "uhd";
    }
    try
    {
        rx_antenna = QString(cfg.lookup("rx_antenna"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_antenna = "RX2";
    }
    try
    {
        tx_antenna = QString(cfg.lookup("tx_antenna"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        tx_antenna = "TX/RX";
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
        bb_gain = 1;
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
        tx_volume = 50;
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
        scan_step = cfg.lookup("scan_step");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        scan_step = 0;
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
        show_fft = cfg.lookup("show_fft");
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        show_fft = 1;
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

}

void Settings::saveConfig()
{
    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();
    root.add("rx_device_args",libconfig::Setting::TypeString) = rx_device_args.toStdString();
    root.add("tx_device_args",libconfig::Setting::TypeString) = tx_device_args.toStdString();
    root.add("rx_antenna",libconfig::Setting::TypeString) = rx_antenna.toStdString();
    root.add("tx_antenna",libconfig::Setting::TypeString) = tx_antenna.toStdString();
    root.add("rx_freq_corr",libconfig::Setting::TypeInt) = rx_freq_corr;
    root.add("tx_freq_corr",libconfig::Setting::TypeInt) = tx_freq_corr;
    root.add("callsign",libconfig::Setting::TypeString) = callsign.toStdString();
    root.add("video_device",libconfig::Setting::TypeString) = video_device.toStdString();
    root.add("audio_input_device",libconfig::Setting::TypeString) = audio_input_device.toStdString();
    root.add("audio_output_device",libconfig::Setting::TypeString) = audio_output_device.toStdString();
    root.add("tx_power",libconfig::Setting::TypeInt) = tx_power;
    root.add("bb_gain",libconfig::Setting::TypeInt) = bb_gain;
    root.add("rx_sensitivity",libconfig::Setting::TypeInt) = rx_sensitivity;
    root.add("squelch",libconfig::Setting::TypeInt) = squelch;
    root.add("rx_volume",libconfig::Setting::TypeInt) = rx_volume;
    root.add("tx_volume",libconfig::Setting::TypeInt) = tx_volume;
    root.add("rx_frequency",libconfig::Setting::TypeInt64) = rx_frequency;
    root.add("tx_shift",libconfig::Setting::TypeInt64) = tx_shift;
    root.add("voip_server",libconfig::Setting::TypeString) = voip_server.toStdString();
    root.add("voip_port",libconfig::Setting::TypeInt) = voip_port;
    root.add("rx_mode",libconfig::Setting::TypeInt) = rx_mode;
    root.add("tx_mode",libconfig::Setting::TypeInt) = tx_mode;
    root.add("ip_address",libconfig::Setting::TypeString) = ip_address.toStdString();
    root.add("demod_offset",libconfig::Setting::TypeInt64) = demod_offset;
    root.add("rx_sample_rate",libconfig::Setting::TypeInt64) = rx_sample_rate;
    root.add("scan_step",libconfig::Setting::TypeInt) = scan_step;
    root.add("show_controls",libconfig::Setting::TypeInt) = show_controls;
    root.add("show_constellation",libconfig::Setting::TypeInt) = show_constellation;
    root.add("show_fft",libconfig::Setting::TypeInt) = show_fft;
    root.add("enable_duplex",libconfig::Setting::TypeInt) = enable_duplex;
    root.add("fft_size",libconfig::Setting::TypeInt) = fft_size;
    root.add("waterfall_fps",libconfig::Setting::TypeInt) = waterfall_fps;
    try
    {
        cfg.writeFile(_config_file->absoluteFilePath().toStdString().c_str());
    }
    catch(const libconfig::FileIOException &fioex)
    {
        std::cerr << "I/O error while writing configuration file: " << _config_file->absoluteFilePath().toStdString() << std::endl;
        exit(EXIT_FAILURE);
    }
}

