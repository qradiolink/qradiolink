#include "netdevice.h"

NetDevice::NetDevice(QObject *parent) :
    QObject(parent)
{
    _fd_tun = 0;
    tun_init();
}

int NetDevice::tun_init()
{
    struct ifreq ifr;
    int s, err;
    struct sockaddr_in addr;
    char dev[] = "tunif0";
    if( (_fd_tun = open("/dev/net/tun", O_RDWR)) < 0 )
    {
        qDebug() << "tun device open failed";
        return -1;
    }

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
        qDebug() << "tun ioctl failed";
        close(_fd_tun);
        return err;
    }
    addr.sin_family = AF_INET;
    s = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_IP);
    strcpy(dev, ifr.ifr_name);
    ifr.ifr_addr = *(struct sockaddr *) &addr;
    ifr.ifr_addr.sa_family = AF_INET;
    inet_pton(AF_INET, "10.0.0.1", ifr.ifr_addr.sa_data + 2);
    ioctl(s, SIOCSIFADDR, &ifr);

    inet_pton(AF_INET, "255.255.255.0", ifr.ifr_addr.sa_data + 2);
    ioctl(s, SIOCSIFNETMASK, &ifr);

    ioctl(s, SIOCGIFFLAGS, &ifr);
    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    if( (err = ioctl(s, SIOCSIFFLAGS, &ifr)) < 0 )
    {
        qDebug() << "could not bring tun if up";
        return err;
    }

    return 1;
}

unsigned char* NetDevice::read_buffered(int &nread)
{
    unsigned char *buffer = new unsigned char[1500];
    nread = read(_fd_tun,buffer,1500);
    if(nread < 0)
    {
      qDebug() << "error reading from tun if";
    }
    return buffer;
}
