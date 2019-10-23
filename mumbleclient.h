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
#include <QElapsedTimer>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include "ext/Mumble.pb.h"
#include "ext/PacketDataStream.h"
#include "ext/utils.h"
#include "sslclient.h"
#include "audio/audioencoder.h"
#include "config_defines.h"
#include "settings.h"
#include "station.h"
#include "mumblechannel.h"
#include "logger.h"

typedef QVector<Station*> StationList;
typedef QVector<MumbleChannel*> ChannelList;
class MumbleClient : public QObject
{
    Q_OBJECT
public:
    explicit MumbleClient(const Settings *settings, Logger *logger, QObject *parent = 0);
    ~MumbleClient();


signals:
    void connectedToServer(QString message);
    void disconnected();
    void channelName(QString name);
    void pcmAudio(short *pcm, int size, quint64 session_id);
    void opusAudio(unsigned char *audio, int size, quint64 session_id);
    void onlineStations(StationList stations);
    void newStation(Station* s);
    void newChannels(ChannelList channes);
    void joinedChannel(quint64 channel_id);
    void leftStation(Station*);
    void channelReady(int chan_number);
    void textMessage(QString msg, bool html);
    void commandMessage(QString msg, int sender_id);
    void userSound(Station* s);
    void userSpeaking(quint64 id);
    
public slots:
    void connectToServer(QString address, unsigned port);
    void disconnectFromServer();
    void cleanup();
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
    int unmuteStation(QString radio_id);
    void disconnectFromCall();
    int muteStation(QString radio_id);
    void setMute(bool mute);
    void setSelfMute(bool mute);
    void setSelfDeaf(bool deaf, bool mute);
    void logMessage(QString log_msg);
    void newMumbleMessage(QString msg);
    void newCommandMessage(QString msg, int to_id);

private:
    void sendUDPMessage(quint8 *message, int size);
    void sendProtoMessage(quint8 *message, quint16 type, int size);
    void setupEncryption(quint8 *message, quint64 size);
    void processVersion(quint8 *message, quint64 size);
    void processServerSync(quint8 *message, quint64 size);
    void processChannelState(quint8 *message, quint64 size);
    void processUserState(quint8 *message, quint64 size);
    void processUserRemove(quint8 *message, quint64 size);
    void createVoicePacket(unsigned char *encoded_audio, int packet_size);
    void processIncomingAudioPacket(quint8 *data, quint64 size, quint8 type);
    void decodeAudio(unsigned char *audiobuffer, short audiobuffersize, quint8 type, quint64 session_id);
    void processTextMessage(quint8 *message, quint64 size);

    AudioEncoder *_codec;
    const Settings *_settings;
    Logger *_logger;
    SSLClient *_socket_client;
#ifndef NO_CRYPT
    CryptState *_crypt_state;
#endif
    QVector<Station*> _stations;
    QVector<MumbleChannel*> _channels;

    QString _temp_channel_name;
    bool _encryption_set;
    bool _synchronized;
    bool _authenticated;
    bool _connection_in_progress;
    quint64 _session_id;
    quint64 _channel_id;
    quint64 _sequence_number;
    QElapsedTimer _last_ping_timer;

    /// not used
    std::string _key;
    std::string _client_nonce;
    std::string _server_nonce;
};


#endif // MUMBLECLIENT_H
