// Written by Adrian Musceac YO8RZZ , started August 2023.
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

#include "udpclient.h"

UDPClient::UDPClient(const Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _started = false;
    _udp_socket_tx = new QUdpSocket(this);
    _udp_socket_rx = new QUdpSocket(this);
}

UDPClient::~UDPClient()
{
    delete _udp_socket_tx;
    delete _udp_socket_rx;
}

void UDPClient::start()
{
    if(_started)
        return;
    int err;
    _resampler_tx = speex_resampler_init(1, _settings->udp_audio_sample_rate, 8000, 10, &err);
    if(err < 0)
        return;
    _resampler_rx = speex_resampler_init(1, 8000, _settings->udp_audio_sample_rate, 10, &err);
    if(err < 0)
        return;
    bool status;
    if(_settings->svx_listen_port != 0)
    {
        status = _udp_socket_tx->bind(QHostAddress::LocalHost, _settings->svx_listen_port);
        _logger->log(Logger::LogLevelInfo, QString(
            "Listening for UDP audio samples on localhost port %1").arg(_settings->svx_listen_port));
    }
    else
        status = false;
    if(!status)
    {
        _logger->log(Logger::LogLevelWarning, QString(
            "Server could not bind to port %1, another instance is probably listening already"
            ).arg(_settings->svx_listen_port));
        _started = false;
    }
    else
    {
        QObject::connect(_udp_socket_tx, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
        _started = true;
    }

}

void UDPClient::stop()
{
    if(!_started)
        return;
    _started = false;

     QObject::disconnect(_udp_socket_tx,SIGNAL(readyRead()),this,SLOT(readPendingDatagrams()));
     _udp_socket_tx->close();
     speex_resampler_destroy(_resampler_tx);
     speex_resampler_destroy(_resampler_rx);
     _logger->log(Logger::LogLevelInfo, "Stop listening for UDP samples");
}

void UDPClient::enable(bool value)
{
    if(value)
        start();
    else
        stop();
}


void UDPClient::readPendingDatagrams()
{
    while (_udp_socket_tx->hasPendingDatagrams())
    {
        QNetworkDatagram datagram = _udp_socket_tx->receiveDatagram();
        if(datagram.isValid())
        {
            QByteArray data = datagram.data();
            _logger->log(Logger::LogLevelDebug, QString("UDP datagram with size: %1").arg(data.size()));
            int16_t *udp_samples = (int16_t*)data.data();
            uint32_t samples = data.size() / sizeof(short);
            int16_t *pcm = new int16_t[samples];
            uint32_t out_length;
            // SVXlink uses a sample rate of 16000 internally, so resample it to our rate of 8000
            speex_resampler_process_int(_resampler_tx, 0, udp_samples, &samples, pcm, &out_length);
            emit pcmAudio(pcm, out_length, 9991);
        }
    }
}

void UDPClient::writeAudioToNetwork(short *pcm, int samples)
{
    uint32_t out_length;
    int16_t *resampled = new int16_t[samples * (_settings->udp_audio_sample_rate / 8000)];
    speex_resampler_process_int(_resampler_rx, 0, pcm, (uint32_t*)&samples, resampled, &out_length);
    qint64 size = out_length * sizeof(int16_t);
    char payload[size];
    memcpy(payload, resampled, size);
    delete[] pcm;
    delete[] resampled;
    _udp_socket_rx->writeDatagram((const char*)payload, size, QHostAddress::LocalHost, _settings->svx_listen_port - 1);
}
