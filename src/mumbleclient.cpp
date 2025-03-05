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

#include "mumbleclient.h"


MumbleClient::MumbleClient(const Settings *settings, Logger *logger, QObject *parent) :
    QObject(parent)
{
    _socket_client = new SSLClient;
    _codec = new AudioEncoder(settings);
    _ping_timer = new QTimer;
    _settings = settings;
    _logger = logger;
    _encryption_set = false;
    _authenticated = false;
    _synchronized = false;
    _session_id = INT64_MAX;
    _channel_id = INT64_MAX;
    _temp_channel_name = "";
    _sequence_number = 0;
    _connection_in_progress = false;

#ifndef NO_CRYPT
    _crypt_state = new CryptState;
#endif
}

MumbleClient::~MumbleClient()
{
    _logger->log(Logger::LogLevelInfo, QString("Shutting down Mumble client"));
    if(_connection_in_progress)
        disconnectFromServer();

    delete _socket_client;
    delete _codec;
    delete _ping_timer;
#ifndef NO_CRYPT
    delete _crypt_state;
#endif
}

void MumbleClient::connectToServer(QString address, unsigned port)
{
    if(_connection_in_progress)
        return;
    _connection_in_progress = true;
    QObject::connect(_socket_client,SIGNAL(connectedToHost()),
                     this,SLOT(sendVersion()));
    QObject::connect(_socket_client,SIGNAL(haveMessage(QByteArray)),
                     this,SLOT(processProtoMessage(QByteArray)));
    QObject::connect(_socket_client,SIGNAL(haveUDPData(QByteArray)),
                     this,SLOT(processUDPData(QByteArray)));
    QObject::connect(_socket_client,SIGNAL(logMessage(QString)),
                     this,SLOT(logMessage(QString)));
    QObject::connect(_socket_client,SIGNAL(connectionFailure()),this,SLOT(cleanup()));
    _socket_client->connectHost(address,port);
    QObject::connect(_ping_timer,SIGNAL(timeout()), this,SLOT(pingServer()));
}

void MumbleClient::disconnectFromServer()
{
    _connection_in_progress = false;
    _socket_client->disconnectHost();
    QObject::disconnect(_socket_client,SIGNAL(connectedToHost()),
                        this,SLOT(sendVersion()));
    QObject::disconnect(_socket_client,SIGNAL(haveMessage(QByteArray)),
                        this,SLOT(processProtoMessage(QByteArray)));
    QObject::disconnect(_socket_client,SIGNAL(haveUDPData(QByteArray)),
                        this,SLOT(processUDPData(QByteArray)));
    QObject::disconnect(_socket_client,SIGNAL(logMessage(QString)),
                        this,SLOT(logMessage(QString)));
    QObject::disconnect(_ping_timer,SIGNAL(timeout()), this,SLOT(pingServer()));
    cleanup();
}

void MumbleClient::cleanup()
{
    _encryption_set = false;
    _authenticated = false;
    _synchronized = false;
    _ping_timer->stop();
    Settings *settings = const_cast<Settings*>(_settings);
    settings->voip_connected = false;
    settings->current_voip_channel = -1;
    _session_id = INT64_MAX;
    _channel_id = INT64_MAX;
    for(int i=0; i < _channels.size();i++)
    {
        delete _channels[i];
    }
    _channels.clear();
    for(int i=0; i < _stations.size();i++)
    {
        delete _stations[i];
    }
    _stations.clear();
    emit disconnected();
    emit textMessage("Disconnected\n", false);
}

void MumbleClient::sendVersion()
{
    MumbleProto::Version *v = new MumbleProto::Version;
    v->set_version(PROTOCOL_VERSION);
    v->set_release("1.0");
    v->set_os("GNU/Linux");
    v->set_os_version("x.x");
    int size = v->ByteSizeLong();
    quint8 data[size];
    v->SerializeToArray(data,size);
    quint16 type = 0;
    sendProtoMessage(data,type,size);
    authenticate();
    delete v;
}

void MumbleClient::authenticate()
{
    emit textMessage("Authenticating\n", false);
    MumbleProto::Authenticate *auth = new MumbleProto::Authenticate;
    QString username = _settings->callsign;
    auth->set_username(username.toStdString());
    auth->set_password(_settings->voip_password.toStdString());
    auth->set_opus(true);
    int size = auth->ByteSizeLong();
    unsigned char data[size];
    auth->SerializeToArray(data,size);
    quint16 type = 2;
    sendProtoMessage(data,type,size);
    delete auth;
}

void MumbleClient::pingServer()
{
    if(!_authenticated)
        return;
    struct timeval now;
    gettimeofday(&now, NULL);
    quint64 ts=now.tv_sec*1000000+now.tv_usec;
    MumbleProto::Ping ping;
    ping.set_timestamp(ts);
    int size = ping.ByteSizeLong();
    quint8 data[size];
    ping.SerializeToArray(data,size);
    sendProtoMessage(data,3,size);
    if(!_settings->_mumble_tcp) // never
        sendUDPPing();
}


void MumbleClient::processProtoMessage(QByteArray data)
{
    int total_size = data.size();
    if(total_size > 4000)
        return;
    quint8 *bin_data = new quint8[total_size];
    unsigned char *temp = reinterpret_cast<unsigned char*>(data.data());
    memcpy(bin_data,temp,total_size);
    int type, len;
    getPreamble(bin_data,&type,&len);
    int message_size = total_size-6;
    quint8 *message = new quint8[message_size];
    memcpy(message,bin_data+6,message_size);
    delete[] bin_data;
    switch(type)
    {
    case 0: // Version
        processVersion(message,message_size);
        break;
    case 4: // Reject
        processReject(message,message_size);
        break;
    case 15:
        setupEncryption(message,message_size);
        break;
    case 5: // ServerSync
        processServerSync(message,message_size);
        break;
    case 3: // ping
        break;
    case 7: // ChannelState
        processChannelState(message, message_size);
        break;
    case 9: // UserState
        processUserState(message, message_size);
        break;
    case 8: // UserRemove
        processUserRemove(message, message_size);
        break;
    case 1: // UDPTunnel
        processIncomingAudioPacket(message, message_size, type);
        break;
    case 11: // TextMessage
        processTextMessage(message, message_size);
        break;
    case 24: // ServerConfig
        processServerConfig(message, message_size);
        break;
    default:
        _logger->log(Logger::LogLevelDebug, QString("Mumble message type %1 not implemented").arg(
                                                        type));
        break;
    }
    delete[] message;
}

void MumbleClient::processVersion(quint8 *message, quint64 size)
{
    QString proto_version, release;
    MumbleProto::Version v;
    v.ParseFromArray(message,size);
    if(v.has_version())
    {
        int version = v.version();
        quint8 major = version >> 16;
        quint8 minor = 0xFF & (version >> 8);
        quint8 patch = 0xFF & version;
        proto_version = QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
    }
    if(v.has_release())
        release = QString::fromStdString(v.release());
    emit textMessage("Protocol version: " + proto_version + "\n", false);
    emit textMessage("Release: " + release + "\n", false);
}

void MumbleClient::processReject(quint8 *message, quint64 size)
{
    MumbleProto::Reject r;
    r.ParseFromArray(message,size);
    if(r.type() == MumbleProto::Reject::UsernameInUse)
    {

    }
    QString reason = QString::fromStdString(r.reason());
    emit textMessage("Rejected: " + reason + "\n", false);
}

void MumbleClient::setupEncryption(quint8 *message, quint64 size)
{
    MumbleProto::CryptSetup crypt;
    crypt.ParseFromArray(message,size);
    _key = crypt.key();
    _client_nonce = crypt.client_nonce();
    _server_nonce = crypt.server_nonce();
#ifndef NO_CRYPT
    _crypt_state->setKey(reinterpret_cast<const unsigned char*>(_key.c_str()),
                         reinterpret_cast<const unsigned char*>(_client_nonce.c_str()),
                         reinterpret_cast<const unsigned char*>(_server_nonce.c_str()));
#endif
    _encryption_set = true;
    _authenticated = true;
    pingServer();
}

void MumbleClient::processServerSync(quint8 *message, quint64 size)
{
    MumbleProto::ServerSync sync;
    sync.ParseFromArray(message,size);
    _session_id = sync.session();
    Station *s = new Station;
    s->id = _session_id;
    _stations.append(s);
    int max_bandwidth = sync.max_bandwidth();
    std::string welcome = sync.welcome_text();
    _synchronized = true;
    QString msg;
    msg = QString::fromStdString(welcome)
             + " max bandwidth: " + max_bandwidth
             + " session: " + QString::number(_session_id) + "\n";
    _logger->log(Logger::LogLevelInfo, msg);
    emit connectedToServer(msg);
    _ping_timer->start(10000);
    Settings *settings = const_cast<Settings*>(_settings);
    settings->voip_connected = true;
}

void MumbleClient::processServerConfig(quint8 *message, quint64 size)
{
    MumbleProto::ServerConfig sc;
    sc.ParseFromArray(message,size);
    QString msg = "Maximum bandwidth: " + QString::number(sc.max_bandwidth())
            + "\nMaximum message length: " + QString::number(sc.message_length())
            + "\nMaximum image length: " + QString::number(sc.image_message_length())
            + "\nHTML allowed: " + QString::number(sc.allow_html())
            + "\n" + QString::fromStdString(sc.welcome_text());
    logMessage(msg);
}

void MumbleClient::processChannelState(quint8 *message, quint64 size)
{
    int id, parent_id;
    QString name, description;
    MumbleProto::ChannelState ch;
    ch.ParseFromArray(message,size);
    if(ch.has_channel_id())
        id = ch.channel_id();
    else
        id = -1;
    if(ch.has_parent())
        parent_id = ch.parent();
    else
        parent_id = -1;
    if(ch.has_name())
        name = QString::fromStdString(ch.name());
    else
        name = "";
    if(ch.has_description())
        description = QString::fromStdString(ch.description());
    else
        description = "";
    MumbleChannel *c = new MumbleChannel(id, parent_id, name, description);
    _channels.append(c);
    emit newChannels(_channels);
}

void MumbleClient::processUserState(quint8 *message, quint64 size)
{
    // FIXME: all this needs refactoring
    MumbleProto::UserState us;
    us.ParseFromArray(message,size);
    if((_session_id==INT64_MAX) && (us.has_channel_id()))
    {
        _channel_id = us.channel_id();
        emit textMessage(QString("Joined channel: %1\n").arg(_channel_id), false);
        emit joinedChannel(_channel_id);
        Settings *settings = const_cast<Settings*>(_settings);
        settings->current_voip_channel = _channel_id;
    }
    if(us.session() == _session_id)
    {
        Station *s;
        for(int i=0; i < _stations.size();i++)
        {
            s = _stations.at(i);
            if(s->id == _session_id)
            {
                s->is_user = true;
                break;
            }
        }

        if(us.has_channel_id())
        {
            _channel_id = us.channel_id();
            emit textMessage(QString("Joined channel: %1\n").arg(_channel_id), false);
            emit joinedChannel(_channel_id);
            s->channel_id = us.channel_id();
            Settings *settings = const_cast<Settings*>(_settings);
            settings->current_voip_channel = _channel_id;
        }
    }

    else
    {
        bool set_station = false;
        for(int i=0; i < _stations.size();i++)
        {
            Station *s = _stations.at(i);
            if(s->id == us.session())
            {
                if(us.has_channel_id())
                    s->channel_id = us.channel_id();
                if(us.has_name())
                    s->callsign = QString::fromStdString(us.name());
                if(us.has_self_mute())
                    s->mute = us.self_mute();
                if(us.has_deaf())
                    s->deaf = us.self_deaf();
                if(us.has_comment())
                    s->callsign += QString::fromStdString(us.comment());
                set_station = true;
            }
        }
        if(!set_station)
        {
            Station *s = new Station;
            s->id = us.session();
            if(us.has_name())
                s->callsign = QString::fromStdString(us.name());
            if(us.has_channel_id())
                s->channel_id = us.channel_id();
            if(us.has_self_mute())
                s->mute = us.self_mute();
            if(us.has_deaf())
                s->deaf = us.self_deaf();
            if(us.has_comment())
                s->callsign += QString::fromStdString(us.comment());
            _stations.push_back(s);
        }
    }

    emit onlineStations(_stations);

}

void MumbleClient::processUserRemove(quint8 *message, quint64 size)
{
    MumbleProto::UserRemove us;
    us.ParseFromArray(message,size);
    for(int i=0; i < _stations.size(); i++)
    {
        Station *s = _stations[i];
        if(s->id == us.session())
        {
            _stations.remove(i);
            delete s;
        }
    }
    emit onlineStations(_stations);
}

void MumbleClient::joinChannel(int id)
{
    MumbleProto::UserState us;
    us.set_session(_session_id);
    us.set_self_deaf(false);
    us.set_self_mute(false);
    us.set_channel_id(id);
    us.set_comment(_settings->callsign.toStdString().c_str());
    int size = us.ByteSizeLong();
    quint8 data[size];
    us.SerializeToArray(data,size);
    sendProtoMessage(data,9,size);
    _channel_id = id;
    Settings *settings = const_cast<Settings*>(_settings);
    settings->current_voip_channel = _channel_id;
}

int MumbleClient::muteStation(QString radio_id)
{
    // FIXME: only server admin can do this
    int sessid = 0;
    for(int i =0;i<_stations.size();i++)
    {
        Station *s = _stations.at(i);
        if(s->radio_id == radio_id)
        {
            sessid = s->id;
            if(s->channel_id != _channel_id)
                return -2;
            s->channel_id = -1;
        }
    }
    if(sessid ==0)
        return -1;
    MumbleProto::UserState us;
    us.set_channel_id(1);
    us.set_session(sessid);
    us.set_actor(_session_id);
    us.set_self_mute(true);
    us.set_self_deaf(true);
    int size = us.ByteSizeLong();
    quint8 data[size];
    us.SerializeToArray(data,size);
    sendProtoMessage(data,9,size);
    return 0;
}

int MumbleClient::unmuteStation(QString radio_id)
{
    // FIXME: only server admin can do this
    int sessid = 0;
    for(int i =0;i<_stations.size();i++)
    {
        Station *s = _stations.at(i);
        if(s->radio_id == radio_id)
        {
            sessid = s->id;
            if(s->channel_id > 1)
                return s->channel_id;
            s->channel_id = _channel_id;
        }
    }
    if(sessid ==0)
        return -1;
    MumbleProto::UserState us;
    us.set_channel_id(_channel_id);
    us.set_session(sessid);
    us.set_actor(_session_id);
    us.set_self_deaf(false);
    us.set_self_mute(false);
    int size = us.ByteSizeLong();
    quint8 data[size];
    us.SerializeToArray(data,size);
    sendProtoMessage(data,9,size);
    return 0;
}

void MumbleClient::disconnectFromCall()
{
    // FIXME: ??
    if(_channel_id < 2)
    {
        return;
    }
    _temp_channel_name = "";
    MumbleProto::UserState us;
    us.set_channel_id(1);
    us.set_session(_session_id);
    us.set_self_mute(true);
    us.set_self_deaf(true);
    int size = us.ByteSizeLong();
    quint8 data[size];
    us.SerializeToArray(data,size);
    sendProtoMessage(data,9,size);
}


QString MumbleClient::getChannelName()
{
    return _temp_channel_name;
}

int MumbleClient::getChannelId()
{
    return _channel_id;
}

QString MumbleClient::createChannel(QString channel_name)
{
    QString name;
    if(channel_name == "")
    {
        int rand_len = 8;
        char rand[9];
        genRandomStr(rand,rand_len);
        name = QString::fromLocal8Bit(rand);
    }
    else
    {
        name = channel_name;
    }
    _temp_channel_name = name;
    MumbleProto::ChannelState channel;
    channel.set_parent(0); // FIXME: channel parent?
    channel.set_name(name.toStdString());
    channel.set_temporary(true);
    int size = channel.ByteSizeLong();
    quint8 data[size];
    channel.SerializeToArray(data,size);
    quint16 type = 7;
    sendProtoMessage(data,type,size);
    emit channelName(_temp_channel_name); // ?
    MumbleProto::UserState us;
    us.set_session(_session_id);
    us.set_actor(_session_id);
    int msize = us.ByteSizeLong();
    quint8 mdata[msize];
    us.SerializeToArray(mdata,msize);
    sendProtoMessage(mdata,9,msize);
    return name;
}

void MumbleClient::setSelfMute(bool mute)
{
    if(!_synchronized)
        return;
    MumbleProto::UserState us;
    us.set_self_mute(mute);
    us.set_session(_session_id);
    us.set_actor(_session_id);
    int msize = us.ByteSizeLong();
    quint8 mdata[msize];
    us.SerializeToArray(mdata,msize);
    sendProtoMessage(mdata,9,msize);
}

void MumbleClient::setSelfDeaf(bool deaf, bool mute)
{
    if(!_synchronized)
        return;
    MumbleProto::UserState us;
    us.set_self_mute(mute); // bug in Mumble? sets mute to true always
    us.set_self_deaf(deaf);
    us.set_session(_session_id);
    us.set_actor(_session_id);
    int msize = us.ByteSizeLong();
    quint8 mdata[msize];
    us.SerializeToArray(mdata,msize);
    sendProtoMessage(mdata,9,msize);
}

void MumbleClient::setMute(bool mute)
{
    if(!_synchronized)
        return;
    MumbleProto::UserState us;
    us.set_self_deaf(mute);
    us.set_self_mute(mute);
    us.set_session(_session_id);
    us.set_actor(_session_id);
    int msize = us.ByteSizeLong();
    quint8 mdata[msize];
    us.SerializeToArray(mdata,msize);
    sendProtoMessage(mdata,9,msize);
}

void MumbleClient::newCommandMessage(QString msg, int to_id)
{
    MumbleProto::TextMessage tm;
    tm.set_actor(_session_id);
    tm.add_session(to_id);
    tm.set_message(msg.toStdString());
    int msize = tm.ByteSizeLong();
    quint8 mdata[msize];
    tm.SerializeToArray(mdata,msize);
    sendProtoMessage(mdata,11,msize);
}

void MumbleClient::newMumbleMessage(QString text)
{
    MumbleProto::TextMessage tm;
    tm.add_channel_id(_channel_id);
    tm.add_session(_session_id);
    tm.set_message(text.toStdString());
    int msize = tm.ByteSizeLong();
    quint8 mdata[msize];
    tm.SerializeToArray(mdata,msize);
    sendProtoMessage(mdata,11,msize);
}

void MumbleClient::processTextMessage(quint8 *message, quint64 size)
{
    MumbleProto::TextMessage tm;
    tm.ParseFromArray(message,size);
    QString sender = "None";
    int sender_id = -1;
    bool channel_msg = (tm.channel_id_size() > 0);
    for(int i =0;i<_stations.size();i++)
    {
        Station *s = _stations.at(i);
        if(s->id == tm.actor())
        {
            sender = s->callsign;
            sender_id = s->id;
            break;
        }
    }
    QString text = QString::fromStdString(tm.message());
    QString msg("\n<br/><b>%1%2</b>: %3<br/>\n");
    if(!channel_msg)
        msg = msg.arg("[private] ").arg(sender).arg(text);
    else
        msg = msg.arg("[channel] ").arg(sender).arg(text);

    emit textMessage(msg, true);
    if(_settings->remote_control && !channel_msg)
        emit commandMessage(text, sender_id);
}

void MumbleClient::processPCMAudio(short *audiobuffer, int audiobuffersize)
{
    if(!_synchronized)
    {
        delete[] audiobuffer;
        return;
    }
    int packet_size = 0;
    unsigned char *encoded_audio;
    /// encode the PCM with higher quality and bitrate
    encoded_audio = _codec->encode_opus_voip(audiobuffer, audiobuffersize, packet_size);
    createVoicePacket(encoded_audio, packet_size);
    delete[] encoded_audio;
    delete[] audiobuffer;
}

void MumbleClient::processOpusAudio(unsigned char *opus_packet, int packet_size)
{
    if(!_synchronized)
    {
        delete[] opus_packet;
        return;
    }
    createVoicePacket(opus_packet, packet_size);
    delete[] opus_packet;
}

void MumbleClient::processVideoFrame(unsigned char *video_frame, int frame_size)
{
    if(!_synchronized)
    {
        delete[] video_frame;
        return;
    }
    createVideoPacket(video_frame, frame_size);
    delete[] video_frame;
}

void MumbleClient::createVoicePacket(unsigned char *encoded_audio, int packet_size)
{
    int type = 0;
    if(_settings->_use_codec2) // FIXME: support for Codec2 frames is only in my old fork
        type |= (5 << 5);
    else
        type |= (4 << 5);

    int data_size = 4096;
    char data[data_size];
    data[0] = static_cast<unsigned char>(type);
    PacketDataStream pds(data + 1, data_size-1);
    int nr_of_frames = opus_packet_get_nb_frames(encoded_audio,packet_size);

    pds << _sequence_number;
    int real_packet_size = packet_size;
    /// Not documented anywhere by Mumble:
    /// by incrementing the sequence by more frames than needed, we can force the client
    /// to wait more time before playing the audio and thus eliminate interruptions on slow networks
    _sequence_number += nr_of_frames * 4;
    //packet_size |= 1 << 13;
    pds << packet_size;

    char *audio_packet = reinterpret_cast<char*>(encoded_audio);

    pds.append(audio_packet,real_packet_size);

    unsigned char *bin_data = reinterpret_cast<unsigned char*>(data);
    if(_settings->_mumble_tcp) // TCP tunnel
    {
        sendProtoMessage(bin_data,1,pds.size()+1);
    }
    else // Use UDP. This won't work because of encryption
    {
        sendUDPMessage(bin_data,pds.size()+1);
    }
}

void MumbleClient::createVideoPacket(unsigned char *video_frame, int frame_size)
{
    int type = 0;
    if(_settings->_use_codec2) // FIXME: support for Codec2 frames is only in my old fork
        type |= (5 << 5);
    else
        type |= (4 << 5);

    int data_size = 4096;
    char data[data_size];
    data[0] = static_cast<unsigned char>(type);
    PacketDataStream pds(data + 1, data_size-1);

    pds << 1;
    int real_packet_size = frame_size;
    pds << frame_size;

    char *video_packet = reinterpret_cast<char*>(video_frame);

    pds.append(video_packet,real_packet_size);

    unsigned char *bin_data = reinterpret_cast<unsigned char*>(data);
    if(_settings->_mumble_tcp) // TCP tunnel
    {
        sendProtoMessage(bin_data,1,pds.size()+1);
    }
    else // Use UDP. This won't work because of encryption
    {
        sendUDPMessage(bin_data,pds.size()+1);
    }
}

void MumbleClient::processIncomingAudioPacket(quint8 *data, quint64 size, quint8 type)
{
    PacketDataStream pds(data+1, size-1);
    quint64 seq_number;
    quint64 session;
    quint8 audio_head;
    pds >> session;
    pds >> seq_number;
    pds >> audio_head;
    //audio_head &= 0x1fff;
    if(type == 1) // Received UDPTunnel
    {
        type = audio_head >> 5;
    }
    int frame_size = pds.left();
    if(frame_size <= 0)
    {
        _logger->log(Logger::LogLevelWarning, "Received malformed audio frame");
        return;
    }
    if(frame_size > 4000)
    {
        _logger->log(Logger::LogLevelWarning, "Dropping frame too large");
        return;
    }
    if(frame_size > 1000)
    {
        QByteArray qba = pds.dataBlock(frame_size);
        unsigned char *encoded_video = reinterpret_cast<unsigned char*>(qba.data());
        unsigned char *video_frame = new unsigned char[frame_size];
        memcpy(video_frame, encoded_video, frame_size);
        emit videoFrame(video_frame, frame_size, session);
    }
    else
    {
        QByteArray qba = pds.dataBlock(pds.left());
        unsigned char *encoded_audio = reinterpret_cast<unsigned char*>(qba.data());
        decodeAudio(encoded_audio,frame_size, type, session);
    }
}

void MumbleClient::processUDPData(QByteArray data)
{
    if(!_encryption_set)
        return;
    unsigned char *encrypted = reinterpret_cast<unsigned char*>(data.data());
#ifndef NO_CRYPT
    unsigned char decrypted[1024];
    _crypt_state->decrypt(encrypted, decrypted, data.size());
    int decrypted_length = data.size() - 4;

    processIncomingAudioPacket(decrypted, decrypted_length);
#else
    quint8 type = data.at(0);
    if(type == 32) // UDP ping reply
        return;
    type = type >> 5;
    processIncomingAudioPacket(encrypted, data.size(), type);
#endif
}

void MumbleClient::decodeAudio(unsigned char *audiobuffer,
                               short audiobuffersize, quint8 type, quint64 session_id)
{
    int samples =0;
    short *pcm = nullptr;
    if(type == 5) // never
    {
        pcm = _codec->decode_codec2_1400(audiobuffer,audiobuffersize, samples);
    }
    else // always
    {
        pcm = _codec->decode_opus_voip(audiobuffer,audiobuffersize, samples);
    }
    if(pcm == nullptr)
        return;
    emit pcmAudio(pcm, samples, session_id);
    emit userSpeaking(session_id);
}



void MumbleClient::sendProtoMessage(quint8 *message, quint16 type, int size)
{
    int new_size = size+6;
    quint8 bin_data[new_size];
    //qToBigEndian<quint16>(type, & bin_data[0]);
    //qToBigEndian<quint32>(len, & bin_data[2]);
    memcpy(bin_data+6,message,size);
    addPreamble(bin_data,type,size);
    _socket_client->sendBin(bin_data,new_size);
}

void MumbleClient::sendUDPMessage(quint8 *message, int size)
{

#ifndef NO_CRYPT
    if(_encryption_set)
    {
        int new_size = size+4;
        quint8 bin_data[new_size];
        _crypt_state->encrypt(message,bin_data,size);
        _telnet->sendUDP(bin_data,new_size);
    }
#else
     _socket_client->sendUDP(message,size);
#endif
}

void MumbleClient::sendUDPPing()
{
    if((!_synchronized) || (!_encryption_set))
        return;
    quint8 head = 32;
    struct timeval now;

    gettimeofday(&now, NULL);
    quint64 ts=now.tv_sec*1000000+now.tv_usec;
    quint8 message[sizeof(quint8) + sizeof(quint64)];
    memcpy(message,&head,sizeof(quint8));
    memcpy(message+sizeof(quint8),&ts,sizeof(quint64));
#ifndef NO_CRYPT
    quint8 encrypted[sizeof(quint8) + sizeof(quint64)+4];
    _crypt_state->encrypt(message,encrypted,sizeof(quint8) + sizeof(quint64));

    _telnet->sendUDP(encrypted,sizeof(quint8) + sizeof(quint64)+4);
#else
    _socket_client->sendUDP(message,sizeof(quint8) + sizeof(quint64));
#endif
}

void MumbleClient::logMessage(QString log_msg)
{
    emit textMessage(log_msg + "\n", false);
}
