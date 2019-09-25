#include "radiochannel.h"

RadioChannels::RadioChannels(QObject *parent) :
    QObject(parent)
{
    _memories_file = setupConfig();
    _channels = new QVector<radiochannel>;
}

RadioChannels::~RadioChannels()
{
    _channels->clear();
    delete _channels;
}

QVector<radiochannel>* RadioChannels::getChannels()
{
    return _channels;
}

QFileInfo *RadioChannels::setupConfig()
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


void RadioChannels::readConfig()
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
    libconfig::Setting &root = cfg.getRoot();

    try
    {
        const libconfig::Setting &channels = root["channels"];
        for(int i=0;i<channels.getLength();i++)
        {
            radiochannel chan;
            chan.id = channels[i]["id"];
            chan.rx_frequency = channels[i]["rx_frequency"];
            chan.tx_frequency = channels[i]["tx_frequency"];
            chan.tx_shift = channels[i]["tx_shift"];
            chan.rx_mode = channels[i]["rx_mode"];
            chan.tx_mode = channels[i]["tx_mode"];
            chan.name = std::string(channels[i]["name"].c_str());
            _channels->push_back(chan);
        }
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        std::cerr << "No memory channels found." << std::endl;
        return;
    }
}


void RadioChannels::saveConfig()
{
    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();
    root.add("channels",libconfig::Setting::TypeList);
    for(int i=0;i<_channels->size();i++)
    {
        radiochannel chan = _channels->at(i);
        root["channels"][i]["id"] = chan.id;
        root["channels"][i]["rx_frequency"] = chan.rx_frequency;
        root["channels"][i]["tx_frequency"] = chan.tx_frequency;
        root["channels"][i]["tx_shift"] = chan.tx_shift;
        root["channels"][i]["rx_mode"] = chan.rx_mode;
        root["channels"][i]["tx_mode"] = chan.rx_mode;
        root["channels"][i]["name"] = chan.name;
    }
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
