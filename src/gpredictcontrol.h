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

#ifndef GPREDICTCONTROL_H
#define GPREDICTCONTROL_H

#include <QObject>
#include "settings.h"
#include "logger.h"

class GPredictControl : public QObject
{
    Q_OBJECT
public:
    explicit GPredictControl(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    QString processMessages(QString message, int &action, qint64 &rx_freq, qint64 &tx_freq,
                            qint64 &rx_freq_delta, qint64 &tx_freq_delta);
    enum RadioAction
    {
        TuneRX,
        TuneTX,
        OffsetRX,
        OffsetTX,
        NoAction
    };

signals:

private:
    const Settings* _settings;
    Logger *_logger;
    qint64 _last_rx_frequency;
    qint64 _last_tx_frequency;

};

#endif // GPREDICTCONTROL_H
