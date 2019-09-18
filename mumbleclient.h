// Written by Adrian Musceac YO8RZZ , started October 2013.
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

#ifndef MUMBLECLIENT_H
#define MUMBLECLIENT_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QtEndian>
#include <QCoreApplication>
#include <string>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "ext/Mumble.pb.h"
#include "ext/PacketDataStream.h"
#include "ext/utils.h"
#include "sslclient.h"
#include "audio/audiointerface.h"
#include "audio/audioencoder.h"
#include "config_defines.h"
#include "settings.h"
#include "station.h"
#include "mumblechannel.h"

typedef QVector<Station> StationList;
class MumbleClient : public QObject
{
    Q_OBJECT
public:
    explicit MumbleClient(Settings *settings, QObject *parent = 0);
    ~MumbleClient();


signals:
    void connectedToServer(QString message);
    void channelName(QString name);
    void pcmAudio(short *pcm, int size, quint64 session_id);
    void opusAudio(unsigned char *audio, int size, quint64 session_id);
    void onlineStations(StationList);
    void newStation(Station* s);
    void newChannel(MumbleChannel* chan);
    void leftStation(Station*);
    void channelReady(int chan_number);
    void textMessage(QString msg, bool html);
    void userSound(Station* s);
    void userSpeaking(quint64 id);
    
public slots:
    void connectToServer(QString address, unsigned port);
    void disconnectFromServer();
    void sendVersion();
    void authenticate();
    void pingServer();
    void processProtoMessage(QByteArray data);
    void processUDPData(QByteArray data);
    void sendUDPPing();
    void processPCMAudio(short *audiobuffer, int audiobuffersize);
    void processOpusAudio(unsigned char *opus_packet, int packet_size);
    QString getChannelName();
    int getChannelId();
    QString createChannel(QString channel_name="");
    void joinChannel(int id);
    int callStation(QString radio_id);
    void disconnectFromCall();
    int disconnectStation(QString radio_id);
    void disconnectAllStations();
    void setMute(bool mute);
    void logMessage(QString log_msg);

private:
    void sendUDPMessage(quint8 *message, int size);
    void sendMessage(quint8 *message, quint16 type, int size);
    void setupEncryption(quint8 *message, quint64 size);

    void processServerSync(quint8 *message, quint64 size);
    void processChannelState(quint8 *message, quint64 size);
    void processUserState(quint8 *message, quint64 size);
    void processUserRemove(quint8 *message, quint64 size);
    void createVoicePacket(unsigned char *encoded_audio, int packet_size);
    void processIncomingAudioPacket(quint8 *data, quint64 size, quint8 type);
    void decodeAudio(unsigned char *audiobuffer, short audiobuffersize, quint8 type, quint64 session_id);

    SSLClient *_socket_client;
#ifndef NO_CRYPT
    CryptState *_crypt_state;
#endif
    std::string _key;
    std::string _client_nonce;
    std::string _server_nonce;
    QString _temp_channel_name;
    bool _encryption_set;
    bool _synchronized;
    bool _authenticated;
    int _session_id;
    int _max_bandwidth;
    int _channel_id;
    AudioEncoder *_codec;
    Settings *_settings;
    quint64 _sequence_number;
    QVector<Station*> _stations;

};


#endif // MUMBLECLIENT_H
