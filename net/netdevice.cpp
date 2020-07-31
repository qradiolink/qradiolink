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

#include "netdevice.h"

NetDevice::NetDevice(Logger *logger, QObject *parent, QString ip_address, int mtu) :
    QObject(parent)
{
    _logger = logger;
    _ip_address = ip_address;
    _fd_tun = 0;
    _if_no = 0;
    _socket = -1;
    _mtu = mtu;
    if_list();
}

NetDevice::~NetDevice()
{
    deinit();
}

void NetDevice::deinit()
{
    close(_fd_tun);
}

int NetDevice::tun_init(QString ip_address)
{
    if(_socket != -1)
        return 1; // already initialized
    _ip_address = ip_address;
    int err;
    struct sockaddr_in addr;
    QString dev_str = "tunif" + _ip_address.back();
    char *dev = const_cast<char*>(dev_str.toStdString().c_str());
    if( (_fd_tun = open("/dev/net/tun", O_RDWR)) < 0 )
    {
        _logger->log(Logger::LogLevelCritical, "Failed to open tun device");
        return -1;
    }
    int flags = fcntl(_fd_tun, F_GETFL, 0);
    fcntl(_fd_tun, F_SETFL, flags | O_NONBLOCK);

    memset(&_ifr, 0, sizeof(_ifr));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
    *        IFF_TAP   - TAP device
    *
    *        IFF_NO_PI - Do not provide packet information
    */
    _ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if( *dev )
        strncpy(_ifr.ifr_name, dev, IFNAMSIZ);

    if( (err = ioctl(_fd_tun, TUNSETIFF, (void *) &_ifr)) < 0 )
    {
        _logger->log(Logger::LogLevelWarning,
            "creating net device failed, run setcap \"cap_net_raw,cap_net_admin+eip\" qradiolink");
        close(_fd_tun);
        return err;
    }
    addr.sin_family = AF_INET;
    _socket = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_IP);
    strcpy(dev, _ifr.ifr_name);
    _ifr.ifr_addr = *(struct sockaddr *) &addr;
    _ifr.ifr_addr.sa_family = AF_INET;


    char *ip = const_cast<char*>(_ip_address.toStdString().c_str());
    int ret = inet_pton(AF_INET, ip, _ifr.ifr_addr.sa_data + 2);
    if(ret != 1)
    {
        _logger->log(Logger::LogLevelCritical, QString("The IP address is not valid %1").arg(err));
        close(_fd_tun);
        _socket = -1;
        return err;
    }

    if( (err = ioctl(_socket, SIOCSIFADDR, &_ifr)) < 0)
    {
        _logger->log(Logger::LogLevelCritical, QString("setting address failed %1").arg(err));
        close(_fd_tun);
        _socket = -1;
        return err;
    }

    inet_pton(AF_INET, "255.255.255.0", _ifr.ifr_addr.sa_data + 2);
    if( (err = ioctl(_socket, SIOCSIFNETMASK, &_ifr)) < 0)
    {
        _logger->log(Logger::LogLevelCritical, QString("setting netmask failed %1").arg(err));
        close(_fd_tun);
        _socket = -1;
        return err;
    }

    if( (err = ioctl(_socket, SIOCGIFFLAGS, &_ifr)) < 0)
    {
        _logger->log(Logger::LogLevelCritical, QString("getting flags failed %1").arg(err));
        close(_fd_tun);
        _socket = -1;
        return err;
    }
    _ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);

    //strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    if( (err = ioctl(_socket, SIOCSIFFLAGS, &_ifr)) < 0 )
    {
        int saved_errno = errno;
        _logger->log(Logger::LogLevelCritical,
                     QString("could not bring tap interface up %1").arg(saved_errno));
        close(_fd_tun);
        _socket = -1;
        return err;
    }
    _ifr.ifr_mtu = _mtu;
    if( (err = ioctl(_socket, SIOCSIFMTU, &_ifr)) < 0)
    {
        _logger->log(Logger::LogLevelCritical, QString("setting MTU failed %1").arg(err));
        close(_fd_tun);
        _socket = -1;
        return err;
    }
    _logger->log(Logger::LogLevelInfo, QString("Successfully opened network interface"));
    if_list();

    return 1;
}

int NetDevice::set_mtu(int mtu)
{
    if(_socket == -1)
        return -1;
    int err;
    _ifr.ifr_mtu = mtu;
    if( (err = ioctl(_socket, SIOCSIFMTU, &_ifr)) < 0)
    {
        _logger->log(Logger::LogLevelCritical, QString("setting MTU failed %1").arg(err));
        close(_fd_tun);
        _socket = -1;
        return err;
    }

    return 1;
}

unsigned char* NetDevice::read_buffered(int &nread, int size)
{
    unsigned char *buffer = new unsigned char[size];
    if(_socket == -1)
    {
        nread = 0;
        return buffer;
    }
    // Using MTU of 1500

    nread = read(_fd_tun,buffer,size);
    /*
    if(nread < 0)
    {
     _logger->log(Logger::LogLevelCritical, "error reading from tap interface");
    }
    */
    return buffer;
}

int NetDevice::write_buffered(unsigned char *data, int len)
{
    if(_socket == -1)
        return -1;
    int nwrite = write(_fd_tun,data,len);
    if(nwrite < 0)
    {
        _logger->log(Logger::LogLevelCritical, "error writing to tap interface");
    }
    if(nwrite < len)
    {
        _logger->log(Logger::LogLevelCritical, QString("dropped %1 bytes: ").arg(len - nwrite));
    }
    delete[] data;
    return nwrite;
}

void NetDevice::if_list()
{
    if(_socket == -1)
        return;
    _logger->log(Logger::LogLevelInfo, "Local network interfaces: ");
    char          buf[1024];
    struct ifconf ifc;
    struct ifreq *ifr;
    int           sck;
    int           nInterfaces;
    int           i;

    /* Get a socket handle. */
    sck = socket(AF_INET, SOCK_DGRAM, 0);
    if(sck < 0)
    {
        perror("socket");
    }

    /* Query available interfaces. */
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if(ioctl(sck, SIOCGIFCONF, &ifc) < 0)
    {
        perror("ioctl(SIOCGIFCONF)");
    }

    /* Iterate through the list of interfaces. */
    ifr         = ifc.ifc_req;
    nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
    for(i = 0; i < nInterfaces; i++)
    {
        struct ifreq *item = &ifr[i];
        QString name = QString(item->ifr_name);
        QRegExp pattern("tunif([0-9]){1,2}");
        int pos = pattern.indexIn(name);
        if(pos > -1)
        {
            _if_no = pattern.cap(1).toInt() + 1;
        }
        QString message = "";
        message.append(QString("%1: IP %2").arg(name, QString::fromLocal8Bit(
                     inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr))));


        if(ioctl(sck, SIOCGIFHWADDR, item) < 0)
        {
            perror("ioctl(SIOCGIFHWADDR)");
        }


        if(ioctl(sck, SIOCGIFBRDADDR, item) >= 0)
        {
            message.append(QString(", BROADCAST %1").arg(
                 QString::fromLocal8Bit(
                      inet_ntoa(((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr))));
        }
        _logger->log(Logger::LogLevelInfo, message);

    }

}
