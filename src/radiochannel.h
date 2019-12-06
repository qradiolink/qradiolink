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

#ifndef RADIOCHANNEL_H
#define RADIOCHANNEL_H

#include <QObject>
#include <QDebug>
#include <QVector>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <libconfig.h++>
#include <string>
#include "logger.h"

struct radiochannel
{
    radiochannel() : id(0), rx_frequency(0), tx_frequency(0),
        tx_shift(0), rx_mode(0), tx_mode(0), squelch(0), rx_volume(0),
        tx_power(0), rx_sensitivity(0),rx_ctcss(0), tx_ctcss(0), name(""), skip(0) {}
    int id;
    long long rx_frequency;
    long long tx_frequency;
    long long tx_shift;
    int rx_mode;
    int tx_mode;
    int squelch;
    int rx_volume;
    int tx_power;
    int rx_sensitivity;
    float rx_ctcss;
    float tx_ctcss;
    std::string name;
    int skip;
};

class RadioChannels : public QObject
{
    Q_OBJECT
public:
    explicit RadioChannels(Logger *logger, QObject *parent = 0);
    ~RadioChannels();
    QFileInfo *setupConfig();
    void readConfig();
    void saveConfig();
    QVector<radiochannel*>* getChannels();

signals:

public slots:

private:
    Logger *_logger;
    QFileInfo *_memories_file;
    QVector<radiochannel*> *_channels;

};

#endif // RADIOCHANNEL_H
