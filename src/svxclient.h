#ifndef SVXCLIENT_H
#define SVXCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <speex/speex_resampler.h>
#include "settings.h"
#include "logger.h"

class SVXClient : public QObject
{
    Q_OBJECT
public:
    explicit SVXClient(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~SVXClient();



signals:
    void pcmAudio(short *pcm, int samples, quint64 mixer_channel);

public slots:
    void start();
    void stop();

private slots:
    void readPendingDatagrams();

private:

    QUdpSocket *_udp_socket;
    const Settings *_settings;
    Logger *_logger;
    bool _started;
    SpeexResamplerState *_resampler;

};

#endif // SVXCLIENT_H
