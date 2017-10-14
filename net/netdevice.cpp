#include "netdevice.h"

NetDevice::NetDevice(QObject *parent) :
    QObject(parent)
{
    _fd_tun = 0;
    _if_no = 0;
    if_list();
    tun_init();
}

int NetDevice::tun_init()
{
    struct ifreq ifr;
    int s, err;
    struct sockaddr_in addr;
    QString dev_str = "tunif" + QString::number(_if_no);
    char *dev = const_cast<char*>(dev_str.toStdString().c_str());
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
        qDebug() << "net ioctl failed";
        close(_fd_tun);
        return err;
    }
    addr.sin_family = AF_INET;
    s = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_IP);
    strcpy(dev, ifr.ifr_name);
    ifr.ifr_addr = *(struct sockaddr *) &addr;
    ifr.ifr_addr.sa_family = AF_INET;
    QString ip_str = "10.0.0." + QString::number(_if_no+1);
    char *ip = const_cast<char*>(ip_str.toStdString().c_str());
    inet_pton(AF_INET, ip, ifr.ifr_addr.sa_data + 2);
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

int NetDevice::write_buffered(unsigned char *data, int len)
{
    int nwrite = write(_fd_tun,data,len);
    if(nwrite < 0)
    {
      qDebug() << "error reading from tun if";
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
