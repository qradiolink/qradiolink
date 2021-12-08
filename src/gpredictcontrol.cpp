// Written by Adrian Musceac YO8RZZ , started Jan 2021.
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
            //_logger->log(Logger::LogLevelDebug, QString("GPredict requested RX frequency %1").arg(freq_string));
            qint64 local_freq = _settings->rx_frequency + _settings->demod_offset + _settings->lnb_lo_freq;
            qint64 new_freq = freq_string.toLong();
            qint64 new_freq_delta = new_freq - _last_rx_frequency;
            qint64 local_freq_delta = new_freq - local_freq;
            _last_rx_frequency = new_freq;

            if(std::abs(local_freq_delta) > 50000)
            {
                qint64 freq = new_freq - _settings->demod_offset - _settings->lnb_lo_freq;
                if(freq >= 28000000)
                {
                    action = RadioAction::TuneRX;
                    rx_freq = freq;
                }
            }
            else if(std::abs(new_freq_delta) > 50000)
            {
                action = RadioAction::OffsetRX;
                rx_freq_delta = local_freq_delta;
            }
            else
            {
                action = RadioAction::OffsetRX;
                rx_freq_delta = new_freq_delta;
            }
            reply = true;
        }
        if(msg.startsWith("I ", Qt::CaseSensitive))
        {
            QString freq_string = msg.mid(1).trimmed();
            //_logger->log(Logger::LogLevelDebug, QString("GPredict requested TX frequency %1").arg(freq_string));
            qint64 local_freq = _settings->tx_frequency + _settings->tx_shift;
            qint64 new_freq = freq_string.toLong();
            qint64 new_freq_delta = new_freq - _last_tx_frequency;
            qint64 local_freq_delta = new_freq - local_freq;
            _last_tx_frequency = new_freq;
            if(std::abs(local_freq_delta) > 50000)
            {
                if(new_freq >= 28000000)
                {
                    action = RadioAction::TuneTX;
                    tx_freq = new_freq;
                }
            }
            else if(std::abs(new_freq_delta) > 50000)
            {
                action = RadioAction::OffsetTX;
                tx_freq_delta = local_freq_delta;
            }
            else
            {
                action = RadioAction::OffsetTX;
                tx_freq_delta = new_freq_delta;
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
