#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <libconfig.h++>
#include <iostream>

class Settings
{
public:
    Settings();
    QFileInfo *setupConfig();
    void readConfig(QFileInfo *config_file);
    void saveConfig();
    quint32 _id;
    quint8 _use_mumble;
    quint8 _mumble_tcp;
    quint8 _use_codec2;
    quint8 _use_dtmf;
    float _audio_treshhold;
    float _voice_activation;
    quint16 _voice_activation_timeout;
    quint16 _voice_server_port;
    quint16 _local_udp_port;
    quint16 _control_port;
    quint16 _opus_bitrate;
    quint16 _codec2_bitrate;
    quint8 _enable_vox;
    quint8 _enable_agc;
    quint16 _ident_time;
    QString _radio_id;
    QString _callsign;
    QString _voice_server_ip;

};

#endif // SETTINGS_H
