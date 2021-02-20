#include "gpredictcontrol.h"

GPredictControl::GPredictControl(const Settings* settings, Logger *logger,  QObject *parent) : QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _last_rx_frequency = 0;
    _last_tx_frequency = 0;
}

QString GPredictControl::processMessages(QString message, int &action, qint64 &rx_freq, qint64 &tx_freq,
                                         qint64 &rx_freq_delta, qint64 &tx_freq_delta)
{
    QStringList messages = message.split("\n", QString::SkipEmptyParts);
    action = RadioAction::NoAction;

    bool reply = false;
    for(int i=0; i< messages.size();i++)
    {
        QString msg = messages.at(i);
        if(msg.startsWith("f", Qt::CaseSensitive))
        {
            return QString("f: %1\n").arg(_settings->rx_frequency + _settings->demod_offset + _settings->lnb_lo_freq);
        }
        if(msg.startsWith("i", Qt::CaseSensitive))
        {
            return QString("i: %1\n").arg(_settings->tx_frequency + _settings->lnb_lo_freq);
        }
        if(msg.startsWith("F ", Qt::CaseSensitive))
        {
            QString freq_string = msg.mid(1).trimmed();
            _logger->log(Logger::LogLevelDebug, QString("GPredict requested RX frequency %1").arg(freq_string));
            qint64 local_freq = _settings->rx_frequency + _settings->demod_offset + _settings->lnb_lo_freq;
            qint64 new_freq = freq_string.toLong();
            qint64 freq_delta = new_freq - local_freq;
            if(std::abs(freq_delta) > 50000)
            {
                qint64 freq = new_freq - _settings->demod_offset - _settings->lnb_lo_freq;
                if(freq >= 28000000)
                {
                    action = RadioAction::TuneRX;
                    rx_freq = freq;
                }
            }
            else
            {
                action = RadioAction::OffsetRX;
                rx_freq_delta = freq_delta;
            }
            reply = true;
        }
        if(msg.startsWith("I ", Qt::CaseSensitive))
        {
            QString freq_string = msg.mid(1).trimmed();
            _logger->log(Logger::LogLevelDebug, QString("GPredict requested TX frequency %1").arg(freq_string));
            qint64 freq = freq_string.toLong() - _settings->lnb_lo_freq;
            if(freq >= 28000000)
            {
                action = RadioAction::TuneTX;
                tx_freq = freq;
            }
            reply = true;
        }
        if(msg.startsWith("S ", Qt::CaseSensitive))
        {
            return QString("RPRT 0\n");
        }
        if(reply)
            return QString("RPRT 0\n");
    }
    return QString("RPRT 0\n");
}
