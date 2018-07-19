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
    rx_mode = 0;
    tx_mode = 0;
    ip_address = "";

    voip_server="127.0.0.1";
    _config_file = setupConfig();
}

QFileInfo *Settings::setupConfig()
{
    QDir files = QDir::homePath();

    QFileInfo new_file = files.filePath(".config/qradiolink.cfg");
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
    try
    {

        cfg.lookupValue("rx_freq_corr", rx_freq_corr);
        cfg.lookupValue("tx_freq_corr", tx_freq_corr);

        rx_device_args = QString(cfg.lookup("rx_device_args"));
        tx_device_args = QString(cfg.lookup("tx_device_args"));
        rx_antenna = QString(cfg.lookup("rx_antenna"));
        tx_antenna = QString(cfg.lookup("tx_antenna"));
        callsign = QString(cfg.lookup("callsign"));
        video_device = QString(cfg.lookup("video_device"));
        tx_power = cfg.lookup("tx_power");
        bb_gain = cfg.lookup("bb_gain");
        rx_sensitivity = cfg.lookup("rx_sensitivity");
        squelch = cfg.lookup("squelch");
        rx_volume = cfg.lookup("rx_volume");
        rx_frequency = cfg.lookup("rx_frequency");
        tx_shift = cfg.lookup("tx_shift");
        voip_server = QString(cfg.lookup("voip_server"));
        rx_mode = cfg.lookup("rx_mode");
        tx_mode = cfg.lookup("tx_mode");
        ip_address = QString(cfg.lookup("ip_address"));

    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        rx_device_args = "rtl=0";
        tx_device_args = "uhd";
        rx_antenna = "RX2";
        tx_antenna = "TX/RX";
        rx_freq_corr = 39;
        tx_freq_corr = 0;
        callsign = "CALL";
        video_device = "/dev/video0";
        tx_power = 50;
        bb_gain = 1;
        rx_sensitivity = 90;
        squelch = -70;
        rx_volume = 10;
        rx_frequency = 434000000;
        tx_shift = 0;
        voip_server = "127.0.0.1";
        rx_mode = 0;
        tx_mode = 0;
        ip_address = "10.0.0.1";
        std::cerr << "Settings not found in configuration file." << std::endl;
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
    root.add("tx_power",libconfig::Setting::TypeInt) = tx_power;
    root.add("bb_gain",libconfig::Setting::TypeInt) = bb_gain;
    root.add("rx_sensitivity",libconfig::Setting::TypeInt) = rx_sensitivity;
    root.add("squelch",libconfig::Setting::TypeInt) = squelch;
    root.add("rx_volume",libconfig::Setting::TypeInt) = rx_volume;
    root.add("rx_frequency",libconfig::Setting::TypeInt64) = rx_frequency;
    root.add("tx_shift",libconfig::Setting::TypeInt64) = tx_shift;
    root.add("voip_server",libconfig::Setting::TypeString) = voip_server.toStdString();
    root.add("rx_mode",libconfig::Setting::TypeInt) = rx_mode;
    root.add("tx_mode",libconfig::Setting::TypeInt) = tx_mode;
    root.add("ip_address",libconfig::Setting::TypeString) = ip_address.toStdString();
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

