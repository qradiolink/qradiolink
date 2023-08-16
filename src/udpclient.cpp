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

const uint32_t INTERNAL_AUDIO_SAMP_RATE = 8000;

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
    int err = 0;
    _resampler_tx = speex_resampler_init(1, _settings->udp_audio_sample_rate, INTERNAL_AUDIO_SAMP_RATE, 10, &err);
    if(err < 0)
    {
        _logger->log(Logger::LogLevelFatal, QString("Could not initialize TX resampler, error code: %1").arg(err));
        return;
    }
    err = 0;
    _resampler_rx = speex_resampler_init(1, INTERNAL_AUDIO_SAMP_RATE, _settings->udp_audio_sample_rate, 10, &err);
    if(err < 0)
    {
        _logger->log(Logger::LogLevelFatal, QString("Could not initialize RX resampler, error code: %1").arg(err));
        return;
    }
    bool status;
    if(_settings->udp_listen_port != 0)
    {
        status = _udp_socket_tx->bind(QHostAddress::LocalHost, _settings->udp_listen_port);
    }
    else
        status = false;
    if(!status)
    {
        _logger->log(Logger::LogLevelWarning, QString(
            "Server could not bind to port %1, another instance is probably listening already"
            ).arg(_settings->udp_listen_port));
        _started = false;
    }
    else
    {
        _logger->log(Logger::LogLevelInfo, QString(
            "Listening for UDP audio samples on localhost port %1").arg(_settings->udp_listen_port));
        _logger->log(Logger::LogLevelInfo, QString(
            "Streaming UDP audio to localhost port %1").arg(_settings->udp_send_port));
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
     _logger->log(Logger::LogLevelInfo, QString("Stopped listening for UDP samples on port %1").arg(_settings->udp_listen_port));
     _logger->log(Logger::LogLevelInfo, QString("Stopped streaming UDP audio on port %1").arg(_settings->udp_send_port));
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
            //_logger->log(Logger::LogLevelDebug, QString("UDP datagram with size: %1").arg(data.size()));
            int16_t *udp_samples = (int16_t*)data.data();
            uint32_t samples = data.size() / sizeof(short);
            int16_t *pcm = new int16_t[samples];
            uint32_t out_length = samples / (_settings->udp_audio_sample_rate / INTERNAL_AUDIO_SAMP_RATE);
            // SVXlink uses a sample rate of 16000 /48000 internally, so resample it to our rate of INTERNAL_AUDIO_SAMP_RATE
            speex_resampler_process_int(_resampler_tx, 0, udp_samples, &samples, pcm, &out_length);
            emit pcmAudio(pcm, out_length, 9991);
        }
    }
}

void UDPClient::writeAudioToNetwork(short *pcm, int samples)
{
    if(!_started)
    {
        delete[] pcm;
        return;
    }

    uint32_t out_length = samples * (_settings->udp_audio_sample_rate / INTERNAL_AUDIO_SAMP_RATE);
    int16_t resampled[out_length];
    speex_resampler_process_int(_resampler_rx, 0, pcm, (uint32_t*)&samples, resampled, &out_length);
    qint64 size = out_length * sizeof(int16_t);
    delete[] pcm;
    //_logger->log(Logger::LogLevelDebug, QString("RX UDP datagram with size: %1").arg(size));
    _udp_socket_rx->writeDatagram((const char*)resampled, size, QHostAddress::LocalHost, _settings->udp_send_port);
}

