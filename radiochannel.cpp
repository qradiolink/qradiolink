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

RadioChannels::RadioChannels(QObject *parent) :
    QObject(parent)
{
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
            radiochannel *chan = new radiochannel;
            chan->id = channels[i]["id"];
            chan->rx_frequency = channels[i]["rx_frequency"];
            chan->tx_frequency = channels[i]["tx_frequency"];
            chan->tx_shift = channels[i]["tx_shift"];
            chan->rx_mode = channels[i]["rx_mode"];
            chan->tx_mode = channels[i]["tx_mode"];
            std::string name(channels[i]["name"].c_str());
            chan->name = name;
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
        radiochannel *chan = _channels->at(i);

        libconfig::Setting &channel = root["channels"].add(libconfig::Setting::TypeGroup);

        channel.add("id", libconfig::Setting::TypeInt) = chan->id;
        channel.add("rx_frequency", libconfig::Setting::TypeInt64) = chan->rx_frequency;
        channel.add("tx_frequency", libconfig::Setting::TypeInt64) = chan->tx_frequency;
        channel.add("tx_shift", libconfig::Setting::TypeInt64) = chan->tx_shift;
        channel.add("rx_mode", libconfig::Setting::TypeInt) = chan->rx_mode;
        channel.add("tx_mode", libconfig::Setting::TypeInt) = chan->rx_mode;
        channel.add("name", libconfig::Setting::TypeString) = chan->name;
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
