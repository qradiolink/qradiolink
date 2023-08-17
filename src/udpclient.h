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

#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <speex/speex_resampler.h>
#include "settings.h"
#include "logger.h"

class UDPClient : public QObject
{
    Q_OBJECT
public:
    explicit UDPClient(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~UDPClient();

signals:
    void pcmAudio(short *pcm, int samples, quint64 mixer_channel);

public slots:
    void enable(bool);
    void writeAudioToNetwork(short *pcm, int samples);

private slots:
    void readPendingDatagrams();

private:
    void start();
    void stop();
    QUdpSocket *_udp_socket_tx;
    QUdpSocket *_udp_socket_rx;
    const Settings *_settings;
    Logger *_logger;
    bool _started;
    SpeexResamplerState *_resampler_tx;
    SpeexResamplerState *_resampler_rx;

};

#endif // UDPCLIENT_H
