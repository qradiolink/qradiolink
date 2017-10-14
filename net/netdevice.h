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
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>

class NetDevice : public QObject
{
    Q_OBJECT
public:
    explicit NetDevice(QObject *parent = 0);

signals:

public slots:

public:
    unsigned char* read_buffered(int &bytes);
    int write_buffered(unsigned char* data, int len);

private:
    int tun_init();
    void if_list();
    int _fd_tun;
    int _if_no;

};

#endif // NETDEVICE_H
