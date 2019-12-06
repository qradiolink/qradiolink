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

NetDevice::NetDevice(Logger *logger, QObject *parent, QString ip_address) :
    QObject(parent)
{
    _logger = logger;
    _fd_tun = 0;
    _if_no = 0;
    if_list();
    tun_init(ip_address);
}

NetDevice::~NetDevice()
{
    close(_fd_tun);
}

int NetDevice::tun_init(QString ip_address)
{
    struct ifreq ifr;
    int s, err;
    struct sockaddr_in addr;
    QString dev_str = "tunif" + QString::number(_if_no);
    char *dev = const_cast<char*>(dev_str.toStdString().c_str());
    if( (_fd_tun = open("/dev/net/tun", O_RDWR)) < 0 )
    {
        _logger->log(Logger::LogLevelCritical, "Failed to open tun device");
        return -1;
    }
    int flags = fcntl(_fd_tun, F_GETFL, 0);
    fcntl(_fd_tun, F_SETFL, flags | O_NONBLOCK);

    memset(&ifr, 0, sizeof(ifr));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
    *        IFF_TAP   - TAP device
    *
    *        IFF_NO_PI - Do not provide packet information
    */
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if( *dev )
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if( (err = ioctl(_fd_tun, TUNSETIFF, (void *) &ifr)) < 0 )
    {
        _logger->log(Logger::LogLevelWarning,
            "creating net device failed, run setcap \"cap_net_raw,cap_net_admin+eip\" qradiolink");
        close(_fd_tun);
        return err;
    }
    addr.sin_family = AF_INET;
    s = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_IP);
    strcpy(dev, ifr.ifr_name);
    ifr.ifr_addr = *(struct sockaddr *) &addr;
    ifr.ifr_addr.sa_family = AF_INET;


    char *ip = const_cast<char*>(ip_address.toStdString().c_str());
    int ret = inet_pton(AF_INET, ip, ifr.ifr_addr.sa_data + 2);
    if(ret != 1)
    {
        _logger->log(Logger::LogLevelCritical, QString("The IP address is not valid %1").arg(err));
        close(_fd_tun);
        return err;
    }

    if( (err = ioctl(s, SIOCSIFADDR, &ifr)) < 0)
    {
        _logger->log(Logger::LogLevelCritical, QString("setting address failed %1").arg(err));
        close(_fd_tun);
        return err;
    }

    inet_pton(AF_INET, "255.255.255.0", ifr.ifr_addr.sa_data + 2);
    if( (err = ioctl(s, SIOCSIFNETMASK, &ifr)) < 0)
    {
        _logger->log(Logger::LogLevelCritical, QString("setting netmask failed %1").arg(err));
        close(_fd_tun);
        return err;
    }

    if( (err = ioctl(s, SIOCGIFFLAGS, &ifr)) < 0)
    {
        _logger->log(Logger::LogLevelCritical, QString("getting flags failed %1").arg(err));
        close(_fd_tun);
        return err;
    }
    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);

    //strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    if( (err = ioctl(s, SIOCSIFFLAGS, &ifr)) < 0 )
    {
        int saved_errno = errno;
        _logger->log(Logger::LogLevelCritical,
                     QString("could not bring tap interface up %1").arg(saved_errno));
        return err;
    }
    ifr.ifr_mtu = 1480;
    if( (err = ioctl(s, SIOCSIFMTU, &ifr)) < 0)
    {
        _logger->log(Logger::LogLevelCritical, QString("setting MTU failed %1").arg(err));
        close(_fd_tun);
        return err;
    }

    return 1;
}

unsigned char* NetDevice::read_buffered(int &nread)
{
    // Using MTU of 1500
    unsigned char *buffer = new unsigned char[1500];
    nread = read(_fd_tun,buffer,1500);
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
        /*
        printf("%s: IP %s",
               name.toStdString().c_str(),
               inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr));


        if(ioctl(sck, SIOCGIFHWADDR, item) < 0)
        {
            perror("ioctl(SIOCGIFHWADDR)");
        }


        if(ioctl(sck, SIOCGIFBRDADDR, item) >= 0)
            printf(", BROADCAST %s", inet_ntoa(((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr));
        printf("\n");
        */
    }

}
