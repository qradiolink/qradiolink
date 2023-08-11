#include "svxclient.h"

SVXClient::SVXClient(const Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _started = false;
    _udp_socket = new QUdpSocket(this);
}

SVXClient::~SVXClient()
{
    delete _udp_socket;
}

void SVXClient::start()
{
    if(_started)
        return;
    int err;
    _resampler = speex_resampler_init(1, 16000, 8000, 5, &err);
    if(err < 0)
        return;
    bool status;
    if(_settings->svx_listen_port != 0)
    {
        status = _udp_socket->bind(QHostAddress::LocalHost, _settings->svx_listen_port);
        _logger->log(Logger::LogLevelInfo, QString(
            "Listening for svxlink connections on localhost port %1").arg(_settings->svx_listen_port));
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
        QObject::connect(_udp_socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
        _started = true;
    }

}

void SVXClient::stop()
{
    if(!_started)
        return;
    _started = false;

     QObject::disconnect(_udp_socket,SIGNAL(readyRead()),this,SLOT(readPendingDatagrams()));
     _udp_socket->close();
     speex_resampler_destroy(_resampler);
}

void SVXClient::readPendingDatagrams()
{
    while (_udp_socket->hasPendingDatagrams())
    {
        QNetworkDatagram datagram = _udp_socket->receiveDatagram();
        if(datagram.isValid())
        {
            QByteArray data = datagram.data();
            _logger->log(Logger::LogLevelDebug, QString("SVX datagram with size: %1").arg(data.size()));
            short *udp_samples = (short*)data.data();
            unsigned int samples = data.size() / sizeof(short);
            short *pcm = new short[samples];
            unsigned int out_length;
            // SVXlink uses a sample rate of 16000 internally, so resample it to our rate of 8000
            speex_resampler_process_int(_resampler, 0, udp_samples, &samples, pcm, &out_length);
            emit pcmAudio(pcm, out_length, 9990);
        }
    }
}
