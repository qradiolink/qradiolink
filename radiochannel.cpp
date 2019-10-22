// Written by Adrian Musceac YO8RZZ , started March 2019.
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

#include "radiochannel.h"

RadioChannels::RadioChannels(Logger *logger, QObject *parent) :
    QObject(parent)
{
    _logger = logger;
    _memories_file = setupConfig();
    _channels = new QVector<radiochannel*>;
}

RadioChannels::~RadioChannels()
{
    for (int i=0;i<_channels->size();i++)
    {
        delete _channels->at(i);
    }
    _channels->clear();
    delete _channels;
}

QVector<radiochannel *> *RadioChannels::getChannels()
{
    return _channels;
}

QFileInfo *RadioChannels::setupConfig()
{
    QDir files = QDir::homePath();
    if(!QDir(files.absolutePath()+"/.config/qradiolink").exists())
    {
        QDir().mkdir(files.absolutePath()+"/.config/qradiolink");
    }
    QFileInfo old_file = files.filePath(".config/qradiolink_mem.cfg");
    if(old_file.exists())
    {
        QDir().rename(old_file.filePath(),
                      files.filePath(".config/qradiolink/qradiolink_mem.cfg"));
    }
    QFileInfo new_file = files.filePath(".config/qradiolink/qradiolink_mem.cfg");
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
        _logger->log(Logger::LogLevelFatal, "I/O error while reading configuration file.");
        exit(EXIT_FAILURE);
    }
    catch(const libconfig::ParseException &pex)
    {
        _logger->log(Logger::LogLevelFatal,
                  QString("Configuration parse error at %1:%2 - %3").arg(
                  pex.getFile()).arg(pex.getLine()).arg(pex.getError()));
        exit(EXIT_FAILURE);
    }

    /// Read memories
    libconfig::Setting &root = cfg.getRoot();
    libconfig::Setting *channels_ptr;
    try
    {
        libconfig::Setting &channels_priv = root["channels"];
        channels_ptr = &channels_priv;
    }
    catch(const libconfig::SettingNotFoundException &nfex)
    {
        _logger->log(Logger::LogLevelInfo, "No memory channels found.");
        return;
    }
    const libconfig::Setting &channels = *channels_ptr;
    for(int i=0;i<channels.getLength();i++)
    {
        radiochannel *chan = new radiochannel;
        try
        {
            chan->id = channels[i]["id"];
            chan->rx_frequency = channels[i]["rx_frequency"];
            chan->tx_frequency = channels[i]["tx_frequency"];
            chan->tx_shift = channels[i]["tx_shift"];
            chan->rx_mode = channels[i]["rx_mode"];
            chan->tx_mode = channels[i]["tx_mode"];
            std::string name(channels[i]["name"].c_str());
            chan->name = name;
        }
        catch (const libconfig::SettingNotFoundException &nfex)
        {
            delete chan;
            continue;
        }
        try
        {
            chan->squelch = channels[i]["squelch"];
            chan->rx_volume = channels[i]["rx_volume"];
            chan->tx_power = channels[i]["tx_power"];
            chan->rx_sensitivity = channels[i]["rx_sensitivity"];
            chan->rx_ctcss = channels[i]["rx_ctcss"];
            chan->tx_ctcss = channels[i]["tx_ctcss"];
        }
        catch (const libconfig::SettingNotFoundException &nfex)
        {
            // FIXME: upgrade config!
        }
        try
        {
            chan->skip = channels[i]["skip"];
        }
        catch (const libconfig::SettingNotFoundException &nfex)
        {
            // FIXME: upgrade config!
        }
        _channels->push_back(chan);
    }

}


void RadioChannels::saveConfig()
{
    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();
    root.add("channels",libconfig::Setting::TypeList);

    for(int i=0;i<_channels->size();i++)
    {
        radiochannel *chan = _channels->at(i);

        libconfig::Setting &channel = root["channels"].add(
                    libconfig::Setting::TypeGroup);

        channel.add("id", libconfig::Setting::TypeInt) = chan->id;
        channel.add("rx_frequency", libconfig::Setting::TypeInt64) = chan->rx_frequency;
        channel.add("tx_frequency", libconfig::Setting::TypeInt64) = chan->tx_frequency;
        channel.add("tx_shift", libconfig::Setting::TypeInt64) = chan->tx_shift;
        channel.add("rx_mode", libconfig::Setting::TypeInt) = chan->rx_mode;
        channel.add("tx_mode", libconfig::Setting::TypeInt) = chan->rx_mode;
        channel.add("squelch", libconfig::Setting::TypeInt) = chan->squelch;
        channel.add("rx_volume", libconfig::Setting::TypeInt) = chan->rx_volume;
        channel.add("tx_power", libconfig::Setting::TypeInt) = chan->tx_power;
        channel.add("rx_sensitivity", libconfig::Setting::TypeInt) = chan->rx_sensitivity;
        channel.add("rx_ctcss", libconfig::Setting::TypeFloat) = chan->rx_ctcss;
        channel.add("tx_ctcss", libconfig::Setting::TypeFloat) = chan->tx_ctcss;
        channel.add("name", libconfig::Setting::TypeString) = chan->name;
        channel.add("skip", libconfig::Setting::TypeInt) = chan->skip;
    }

    try
    {
        cfg.writeFile(_memories_file->absoluteFilePath().toStdString().c_str());
    }
    catch(const libconfig::FileIOException &fioex)
    {
        _logger->log(Logger::LogLevelFatal, "I/O error while writing configuration file: "
                  + _memories_file->absoluteFilePath());
        exit(EXIT_FAILURE);
    }
}
