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

#include "zeromqclient.h"

ZeroMQClient::ZeroMQClient(Settings *settings, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _rx_buffer = new QVector<int16_t>();
    _tx_buffer = new QVector<int16_t>();
    _tx_timeout_counter = 0;
}

ZeroMQClient::~ZeroMQClient()
{
    _rx_buffer->clear();
    delete _rx_buffer;
    _tx_buffer->clear();
    delete _tx_buffer;
}


void ZeroMQClient::init()
{
    _zmqcontextTX = zmq::context_t(1);
    _zmqsocketTX = zmq::socket_t(_zmqcontextTX, ZMQ_REP);
    _zmqsocketTX.set(zmq::sockopt::sndhwm, 10);
    _zmqsocketTX.bind ("ipc:///tmp/mmdvm-tx" + std::to_string(_settings->zmq_proxy_channel) + ".ipc");

    _zmqcontextRX = zmq::context_t(1);
    _zmqsocketRX = zmq::socket_t(_zmqcontextRX, ZMQ_PULL);
    _zmqsocketRX.set(zmq::sockopt::sndhwm, 10);
    _zmqsocketRX.connect ("ipc:///tmp/mmdvm-rx" + std::to_string(_settings->zmq_proxy_channel) + ".ipc");
    ::pthread_mutex_init(&_TXlock, NULL);
    ::pthread_mutex_init(&_RXlock, NULL);

    ::pthread_create(&_threadTX, NULL, tx_thread, this);
    ::pthread_create(&_threadRX, NULL, rx_thread, this);
    ::pthread_setname_np(_threadTX, "zmq_mmdvm_tx");
    ::pthread_setname_np(_threadRX, "zmq_mmdvm_rx");
}

void* ZeroMQClient::tx_thread(void* arg)
{
    ZeroMQClient* o = (ZeroMQClient*)arg;

    while (1)
    {
        o->transmit();
    }

    return NULL;
}

void* ZeroMQClient::rx_thread(void* arg)
{
    ZeroMQClient* o = (ZeroMQClient*)arg;

    while (1)
    {
        o->receive();
    }

    return NULL;
}

void ZeroMQClient::transmit()
{
    uint32_t num_items = 720;
    zmq::message_t request_message;
    zmq::recv_result_t recv_result = _zmqsocketTX.recv(request_message);
    Q_UNUSED(recv_result);
    if(request_message.size() < 1)
        return;
    ::pthread_mutex_lock(&_TXlock);
    _tx_mutex.lock();
    if((uint32_t)_tx_buffer->size() >= num_items)
    {

        int buf_size = sizeof(uint32_t) + num_items * sizeof(uint8_t) + num_items * sizeof(int16_t);

        zmq::message_t reply (buf_size);
        memcpy (reply.data (), &num_items, sizeof(uint32_t));
        memset ((unsigned char *)reply.data () + sizeof(uint32_t), 0, num_items * sizeof(uint8_t));
        memcpy ((unsigned char *)reply.data () + sizeof(uint32_t) + num_items * sizeof(uint8_t),
                (unsigned char *)_tx_buffer->begin(), num_items*sizeof(int16_t));
        _zmqsocketTX.send (reply, zmq::send_flags::dontwait);
        _tx_buffer->erase(_tx_buffer->begin(), _tx_buffer->begin()+num_items);

        ::pthread_mutex_unlock(&_TXlock);
        _tx_mutex.unlock();
    }
    else if(_tx_timeout_counter < 10)
    {
        int buf_size = sizeof(uint32_t) + num_items * sizeof(uint8_t) + num_items * sizeof(int16_t);

        zmq::message_t reply (buf_size);
        memcpy (reply.data (), &num_items, sizeof(uint32_t));
        memset ((unsigned char *)reply.data () + sizeof(uint32_t), 0, num_items * sizeof(uint8_t));
        memset ((unsigned char *)reply.data () + sizeof(uint32_t) + num_items * sizeof(uint8_t),
                0, num_items*sizeof(int16_t));
        _zmqsocketTX.send (reply, zmq::send_flags::dontwait);
        ::pthread_mutex_unlock(&_TXlock);
        _tx_timeout_counter++;
        _tx_mutex.unlock();
    }
    else
    {
        ::pthread_mutex_unlock(&_TXlock);
        _tx_mutex.unlock();
        zmq::message_t reply (sizeof(uint32_t));
        uint32_t items = 0;
        memcpy (reply.data (), &items, sizeof(uint32_t));
        _zmqsocketTX.send (reply, zmq::send_flags::dontwait);
    }
}

void ZeroMQClient::receive()
{
    uint32_t num_items = 720;
    zmq::message_t mq_message;
    zmq::recv_result_t recv_result = _zmqsocketRX.recv(mq_message, zmq::recv_flags::none);
    Q_UNUSED(recv_result);
    int size = mq_message.size();
    uint32_t data_size = 0;
    uint32_t rssi = 0;
    if(size < 1)
        return;
    memcpy(&data_size, (unsigned char*)mq_message.data(), sizeof(uint32_t));
    memcpy(&rssi, (unsigned char*)mq_message.data() + sizeof(uint32_t), sizeof(uint32_t));

    ::pthread_mutex_lock(&_RXlock);
    for(uint32_t i=0;i < data_size;i++)
    {
        int16_t signed_sample = 0;
        memcpy(&signed_sample, (unsigned char*)mq_message.data() + sizeof(uint32_t) + sizeof(uint32_t) + data_size * sizeof(uint8_t) + i * sizeof(int16_t), sizeof(int16_t));
        _rx_buffer->push_back(int16_t((float(signed_sample) * float(_settings->voip_volume) / 100.0f)));
    }
    ::pthread_mutex_unlock(&_RXlock);
    if((uint32_t)_rx_buffer->size() >= num_items * 4) // buffer up to 120 ms
    {
        short *samples = new int16_t[num_items];
        memcpy ((unsigned char *)samples, (unsigned char *)_rx_buffer->begin(), num_items*sizeof(int16_t));
        _rx_buffer->erase(_rx_buffer->begin(), _rx_buffer->begin()+num_items);
        emit rxSamples(samples, num_items);
    }
    return;
}

void ZeroMQClient::txSamples(short *samples, int size, quint64 chan)
{
    Q_UNUSED(chan);
    _tx_mutex.lock();
    _tx_timeout_counter = 0;
    for(int i=0;i< size;i++)
    {
        _tx_buffer->push_back(int16_t(float(samples[i]) * float(_settings->voip_volume) / 100.0f) );
    }
    _tx_mutex.unlock();
    delete[] samples;
}
