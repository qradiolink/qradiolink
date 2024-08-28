// Written by Adrian Musceac YO8RZZ , started August 2024.
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

#ifndef ZEROMQCLIENT_H
#define ZEROMQCLIENT_H

#include <zmq.hpp>
#include <pthread.h>
#include <QObject>
#include <QVector>
#include <QMutex>
#include "settings.h"

class ZeroMQClient : public QObject
{
    Q_OBJECT
public:
    explicit ZeroMQClient(Settings *settings, QObject *parent = nullptr);
    ~ZeroMQClient();
    void init();
    static void* tx_thread(void * arg);
    static void* rx_thread(void * arg);
    void receive();
    void transmit();

signals:
    void rxSamples(short *samples, int size);

public slots:
    void txSamples(short *samples, int size, quint64 chan);

private:
    Settings *_settings;
    zmq::context_t _zmqcontextTX;
    zmq::socket_t _zmqsocketTX;
    zmq::context_t _zmqcontextRX;
    zmq::socket_t _zmqsocketRX;
    pthread_t _threadTX;
    pthread_t _threadRX;
    pthread_mutex_t _TXlock;
    pthread_mutex_t _RXlock;
    QVector<int16_t> *_rx_buffer;
    QVector<int16_t> *_tx_buffer;
    QMutex _tx_mutex;
    QMutex _rx_mutex;

};

#endif // ZEROMQCLIENT_H
