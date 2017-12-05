#include "settings.h"

Settings::Settings()
{
    _id = 0;
    _use_mumble = 1;
    _mumble_tcp = 1; // used
    _use_codec2 = 0; // used
    _use_dtmf = 0; // used
    _audio_treshhold = -15; // used
    _voice_activation = 0.5; // used
    _voice_activation_timeout = 50; // used
    _voice_server_port = 64738;
    _local_udp_port = 4938;
    _control_port = 4939;
    _opus_bitrate = 8000;
    _codec2_bitrate =1200;
    _enable_vox = 0; // unused
    _enable_agc = 0; // unused
    _ident_time = 300; // used
    _radio_id = "";
    _callsign = "";
    _voice_server_ip="127.0.0.1";
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

/*
void Settings::readConfig(QFileInfo *config_file)
{
    libconfig::Config cfg;
    try
    {
        cfg.readFile(config_file->absoluteFilePath().toStdString().c_str());
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
        int rx_freq_corr, tx_freq_corr;
        cfg.lookupValue("rx_freq_corr", rx_freq_corr);
        cfg.lookupValue("tx_freq_corr", tx_freq_corr);

        ui->lineEditRXDev->setText(QString(cfg.lookup("rx_device_args")));
        ui->lineEditTXDev->setText(QString(cfg.lookup("tx_device_args")));
        ui->lineEditRXAntenna->setText(QString(cfg.lookup("rx_antenna")));
        ui->lineEditTXAntenna->setText(QString(cfg.lookup("tx_antenna")));
        ui->lineEditRXFreqCorrection->setText(QString::number(rx_freq_corr));
        ui->lineEditTXFreqCorrection->setText(QString::number(tx_freq_corr));
        ui->lineEditCallsign->setText(QString(cfg.lookup("callsign")));
        ui->lineEditVideoDevice->setText(QString(cfg.lookup("video_device")));
        ui->txPowerSlider->setValue(cfg.lookup("tx_power"));
        ui->rxSensitivitySlider->setValue(cfg.lookup("rx_sensitivity"));
        ui->rxSquelchSlider->setValue(cfg.lookup("squelch"));
        ui->rxVolumeSlider->setValue(cfg.lookup("rx_volume"));
        ui->frameCtrlFreq->setFrequency(cfg.lookup("rx_frequency"));
        _rx_frequency = cfg.lookup("rx_frequency");
        _tx_frequency = cfg.lookup("tx_shift");
        ui->shiftEdit->setText(QString::number(_tx_frequency / 1000));

    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        ui->lineEditRXDev->setText("rtl=0");
        ui->lineEditTXDev->setText("uhd");
        ui->lineEditRXAntenna->setText("RX2");
        ui->lineEditTXAntenna->setText("TX/RX");
        ui->lineEditRXFreqCorrection->setText("39");
        ui->lineEditTXFreqCorrection->setText("0");
        ui->lineEditCallsign->setText("CALL");
        ui->lineEditVideoDevice->setText("/dev/video0");
        _rx_frequency = 434000000;
        ui->frameCtrlFreq->setFrequency(_rx_frequency);
        _tx_frequency = 0;
        ui->shiftEdit->setText(QString::number(_tx_frequency,10));

        std::cerr << "Settings not found in configuration file." << std::endl;
    }
}

void Settings::saveConfig()
{
    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();
    root.add("rx_device_args",libconfig::Setting::TypeString) = ui->lineEditRXDev->text().toStdString();
    root.add("tx_device_args",libconfig::Setting::TypeString) = ui->lineEditTXDev->text().toStdString();
    root.add("rx_antenna",libconfig::Setting::TypeString) = ui->lineEditRXAntenna->text().toStdString();
    root.add("tx_antenna",libconfig::Setting::TypeString) = ui->lineEditTXAntenna->text().toStdString();
    root.add("rx_freq_corr",libconfig::Setting::TypeInt) = ui->lineEditRXFreqCorrection->text().toInt();
    root.add("tx_freq_corr",libconfig::Setting::TypeInt) = ui->lineEditTXFreqCorrection->text().toInt();
    root.add("callsign",libconfig::Setting::TypeString) = ui->lineEditCallsign->text().toStdString();
    root.add("video_device",libconfig::Setting::TypeString) = ui->lineEditVideoDevice->text().toStdString();
    root.add("tx_power",libconfig::Setting::TypeInt) = (int)ui->txPowerSlider->value();
    root.add("rx_sensitivity",libconfig::Setting::TypeInt) = (int)ui->rxSensitivitySlider->value();
    root.add("squelch",libconfig::Setting::TypeInt) = (int)ui->rxSquelchSlider->value();
    root.add("rx_volume",libconfig::Setting::TypeInt) = (int)ui->rxVolumeSlider->value();
    root.add("rx_frequency",libconfig::Setting::TypeInt64) = _rx_frequency;
    root.add("tx_shift",libconfig::Setting::TypeInt64) = _tx_frequency;
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
*/
