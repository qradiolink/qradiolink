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
#include <iostream>
#include <string>

struct radiochannel
{
    radiochannel() : id(0), rx_frequency(0), tx_frequency(0), tx_shift(0), rx_mode(0), tx_mode(0), name("") {}
    int id;
    long long rx_frequency;
    long long tx_frequency;
    long long tx_shift;
    int rx_mode;
    int tx_mode;
    std::string name;
};

class RadioChannels : public QObject
{
    Q_OBJECT
public:
    explicit RadioChannels(QObject *parent = 0);
    ~RadioChannels();
    QFileInfo *setupConfig();
    void readConfig();
    void saveConfig();
    QVector<radiochannel*>* getChannels();

signals:

public slots:

private:
    QFileInfo *_memories_file;
    QVector<radiochannel*> *_channels;

};

#endif // RADIOCHANNEL_H
