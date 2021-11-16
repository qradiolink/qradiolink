#ifndef LIMERFECONTROLLER_H
#define LIMERFECONTROLLER_H

#include <QObject>
#include <lime/limeRFE.h>
// Written by Adrian Musceac YO8RZZ , started November 2021.
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

#include "settings.h"
#include "limits.h"

class LimeRFEController : public QObject
{
    Q_OBJECT
public:
    explicit LimeRFEController(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~LimeRFEController();
    void setRXBand(int64_t rx_frequency);
    void setTXBand(int64_t tx_frequency);
    void setDuplex(bool duplex_mode);
    void setTransmit(bool tx_on);
    void setAttenuator(int value);
    void setNotchFilter(bool enable);


signals:

public slots:
    void init();
    void deinit();

private:
    QString getError(int res);
    Logger *_logger;
    Limits *_limits;
    const Settings *_settings;
    bool _lime_rfe_inited;
    rfe_dev_t *_lime_rfe;
    rfe_boardState _board_state;
    bool _duplex_mode;
    bool _transmit_on;
    int _current_rx_band;
    int _current_tx_band;
    bool _low_band_tx; // used only for TX port when switching RF paths in duplex mode

};

#endif // LIMERFECONTROLLER_H
