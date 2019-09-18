#include "radiochannel.h"

RadioChannel::RadioChannel(QObject *parent) :
    QObject(parent)
{
    _memories_file = setupConfig();
}

QFileInfo *RadioChannel::setupConfig()
{
    QDir files = QDir::homePath();
    // FIXME: standard says own directory, plus need to store more configs separately
    QFileInfo new_file = files.filePath(".config/qradiolink_mem.cfg");
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


void RadioChannel::readConfig()
{
    libconfig::Config cfg;
    try
    {
        cfg.readFile(_memories_file->absoluteFilePath().toStdString().c_str());
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

    /// Read memories
    try
    {
        //cfg.lookupValue("rx_freq_corr", rx_freq_corr);
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
    }
    try
    {
        //rx_device_args = QString(cfg.lookup("rx_device_args"));
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {

    }
}


void RadioChannel::saveConfig()
{
    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();
    //root.add("rx_device_args",libconfig::Setting::TypeList) = rx_device_args.toStdString();
    try
    {
        cfg.writeFile(_memories_file->absoluteFilePath().toStdString().c_str());
    }
    catch(const libconfig::FileIOException &fioex)
    {
        std::cerr << "I/O error while writing configuration file: " << _memories_file->absoluteFilePath().toStdString() << std::endl;
        exit(EXIT_FAILURE);
    }
}
