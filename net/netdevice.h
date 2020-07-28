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

#ifndef NETDEVICE_H
#define NETDEVICE_H

#include <QObject>
#include <QDebug>
#include <QRegExp>
#include <QCoreApplication>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "src/logger.h"

class NetDevice : public QObject
{
    Q_OBJECT
public:
    explicit NetDevice(Logger *logger, QObject *parent = 0, QString ip_address="", int mtu=1480);
    ~NetDevice();
signals:

public slots:

public:
    unsigned char* read_buffered(int &bytes, int size=1500);
    int write_buffered(unsigned char* data, int len);
    int set_mtu(int mtu=1500);
    int tun_init();
    void deinit();

private:
    Logger *_logger;
    QString _ip_address;
    int tun_init(QString ip_address);
    void if_list();
    int _fd_tun;
    int _if_no;
    int _mtu;
    struct ifreq _ifr;
    int _socket;

};

#endif // NETDEVICE_H
