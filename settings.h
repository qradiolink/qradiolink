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
    void readConfig();
    void saveConfig();
    QString rx_device_args;
    QString tx_device_args;
    QString rx_antenna;
    QString tx_antenna;
    int tx_power;
    int rx_sensitivity;
    int rx_freq_corr;
    int tx_freq_corr;
    int squelch;
    int rx_volume;
    long long rx_frequency;
    long long tx_shift;
    QString callsign;
    QString video_device;
    QString voip_server;
    int rx_mode;
    int tx_mode;
    QString ip_address;


    quint32 _id;
    quint8 _mumble_tcp;
    quint8 _use_codec2;
    quint8 _use_dtmf;
    float _audio_treshhold;
    float _voice_activation;
    quint16 _voice_activation_timeout;
    quint16 _voice_server_port;
    quint16 _local_udp_port;
    quint16 _control_port;
    quint8 _enable_vox;
    quint8 _enable_agc;
    quint16 _ident_time;
    QString _radio_id;

private:
    QFileInfo *_config_file;
};

#endif // SETTINGS_H
