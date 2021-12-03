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

#include "limerfecontroller.h"

LimeRFEController::LimeRFEController(const Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{

    _logger = logger;
    _settings = settings;
    _lime_rfe_inited = false;
    _duplex_mode = false;
    _transmit_on = false;
    _low_band_tx = false;
    _current_rx_band = 12;
    _current_tx_band = 12;
    _limits = new Limits();

    /// board defaults
    _board_state.selPortRX = RFE_PORT_1;
    _board_state.selPortTX = RFE_PORT_1;
    _board_state.channelIDRX = RFE_CID_HAM_0145;
    _board_state.channelIDTX = RFE_CID_HAM_0145;
    _board_state.mode = RFE_MODE_RX;
    _board_state.notchOnOff = RFE_NOTCH_OFF;
    _board_state.attValue = 0;
    _board_state.enableSWR = RFE_SWR_DISABLE;
    _board_state.sourceSWR = RFE_SWR_SRC_EXT;
}

LimeRFEController::~LimeRFEController()
{
    deinit();
    delete _limits;
}

void LimeRFEController::init()
{
    if(_lime_rfe_inited)
        return;
    _lime_rfe = RFE_Open(_settings->lime_rfe_device.toStdString().c_str(), nullptr);
    if(_lime_rfe == nullptr)
    {
        _logger->log(Logger::LogLevelWarning, "Could not open LimeRFE device");
        return;
    }

    _logger->log(Logger::LogLevelInfo, "Enabling LimeRFE...Succesfully opened LimeRFE");

    // configure defaults
    int res = RFE_ConfigureState(_lime_rfe, _board_state);
    if(res != RFE_SUCCESS)
    {
        _logger->log(Logger::LogLevelCritical, QString("LimeRFE failed configuration %1").arg(getError(res)));
        return;
    }
    _lime_rfe_inited = true;
}

void LimeRFEController::deinit()
{
    if(_lime_rfe_inited)
    {
        RFE_Close(_lime_rfe);
        _lime_rfe_inited = false;
        _logger->log(Logger::LogLevelInfo, "LimeRFE device closed");
    }
}

QString LimeRFEController::getError(int res)
{
    /*
     * See LimeRFE.h
        */
    QString error;
    switch(res)
    {
    case -4:
        error = "Error synchronizing communication";
        break;
    case -3:
        error = "Non-configurable GPIO pin specified. Only pins 4 and 5 are configurable";
        break;
    case -2:
        error = "Problem with .ini configuration file";
        break;
    case -1:
        error = "Communication error";
        break;
    case 1:
        error = "Wrong TX connector - not possible to route TX of the selecrted channel to the specified port";
        break;
    case 2:
        error = "Wrong RX connector - not possible to route RX of the selecrted channel to the specified port";
        break;
    case 3:
        error = "Mode TXRX not allowed - when the same port is selected for RX and TX, it is not allowed to use mode RX & TX";
        break;
    default:
        error = "Unknown error code";
        break;
    }
    return error;
}

void LimeRFEController::setTransmit(bool tx_on)
{
    if(!_lime_rfe_inited)
        return;
    if(tx_on)
    {
        if(_duplex_mode && !_low_band_tx)
        {
            _board_state.mode = RFE_MODE_TXRX;
            int res = RFE_Mode(_lime_rfe, RFE_MODE_TXRX);
            if(res != RFE_SUCCESS)
                _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
        }
        else
        {
            _board_state.mode = RFE_MODE_TX;
            int res = RFE_Mode(_lime_rfe, RFE_MODE_TX);
            if(res != RFE_SUCCESS)
                _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
        }

    }
    else
    {
        _board_state.mode = RFE_MODE_RX;
        int res = RFE_Mode(_lime_rfe, RFE_MODE_RX);
        if(res != RFE_SUCCESS)
            _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
    }
    _transmit_on = tx_on;
}

void LimeRFEController::setDuplex(bool duplex_mode)
{
    /// TODO
    if(!_lime_rfe_inited)
        return;
    if(_transmit_on) // Cannot set Duplex on or off during transmission to avoid getting into a bad state
    {
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE was not set to Duplex mode because it was in TX mode."));
        return;
    }
    if(duplex_mode && _duplex_mode)
        return;
    if(!duplex_mode && !_duplex_mode)
        return;
    if(duplex_mode && !_duplex_mode)
    {
        if(!_low_band_tx)
        {
            _board_state.selPortTX = RFE_PORT_2;
            int res = RFE_ConfigureState(_lime_rfe, _board_state);
            if(res != RFE_SUCCESS)
                _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
            _logger->log(Logger::LogLevelInfo, QString("TX port is now TX (J4)"));
        }
    }
    if(!duplex_mode && _duplex_mode)
    {
        if(!_low_band_tx)
        {
            _board_state.selPortTX = RFE_PORT_1;
            int res = RFE_ConfigureState(_lime_rfe, _board_state);
            if(res != RFE_SUCCESS)
                _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
            _logger->log(Logger::LogLevelInfo, QString("TX port is now TX/RX (J3)"));
        }
    }
    _duplex_mode = duplex_mode;
}

void LimeRFEController::setRXBand(int64_t rx_frequency)
{
    if(!_lime_rfe_inited)
        return;
    if(rx_frequency < 10000)
        return;

    int rx_band = _limits->getRFEBand(rx_frequency);
    if(_current_rx_band == rx_band)
        return;
    _current_rx_band = rx_band;
    switch(rx_band)
    {
    case -1:
        _board_state.channelIDRX = RFE_CID_WB_1000;
        break;
    case -2:
        _board_state.channelIDRX = RFE_CID_WB_4000;
        break;
    case 0:
        _board_state.channelIDRX = RFE_CID_HAM_0030;
        break;
    case 1:
        _board_state.channelIDRX = RFE_CID_HAM_0070;
        break;
    case 2:
        _board_state.channelIDRX = RFE_CID_HAM_0145;
        break;
    case 3:
        _board_state.channelIDRX = RFE_CID_HAM_0220;
        break;
    case 4:
        _board_state.channelIDRX = RFE_CID_HAM_0435;
        break;
    case 5:
        _board_state.channelIDRX = RFE_CID_HAM_0920;
        break;
    case 6:
        _board_state.channelIDRX = RFE_CID_HAM_1280;
        break;
    case 7:
        _board_state.channelIDRX = RFE_CID_HAM_2400;
        break;
    case 8:
        _board_state.channelIDRX = RFE_CID_HAM_3500;
        break;
    default:
        _board_state.channelIDRX = RFE_CID_HAM_0030;
        break;
    }

    if(rx_frequency < 72000000)
    {
        _board_state.selPortRX = RFE_PORT_3;
    }
    else
    {
        _board_state.selPortRX = RFE_PORT_1;
    }

    int res = RFE_ConfigureState(_lime_rfe, _board_state);
    if(res != RFE_SUCCESS)
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
}

void LimeRFEController::setTXBand(int64_t tx_frequency)
{
    if(!_lime_rfe_inited)
        return;
    if(tx_frequency < 10000)
        return;

    int tx_band = _limits->getRFEBand(tx_frequency);
    if(_current_tx_band == tx_band)
        return;
    _current_tx_band = tx_band;

    switch(tx_band)
    {
    case -1:
        _board_state.channelIDTX = RFE_CID_WB_1000;
        break;
    case -2:
        _board_state.channelIDTX = RFE_CID_WB_4000;
        break;
    case 0:
        _board_state.channelIDTX = RFE_CID_HAM_0030;
        break;
    case 1:
        _board_state.channelIDTX = RFE_CID_HAM_0070;
        break;
    case 2:
        _board_state.channelIDTX = RFE_CID_HAM_0145;
        break;
    case 3:
        _board_state.channelIDTX = RFE_CID_HAM_0220;
        break;
    case 4:
        _board_state.channelIDTX = RFE_CID_HAM_0435;
        break;
    case 5:
        _board_state.channelIDTX = RFE_CID_HAM_0920;
        break;
    case 6:
        _board_state.channelIDTX = RFE_CID_HAM_1280;
        break;
    case 7:
        _board_state.channelIDTX = RFE_CID_HAM_2400;
        break;
    case 8:
        _board_state.channelIDTX = RFE_CID_HAM_3500;
        break;
    default:
        _board_state.channelIDTX = RFE_CID_HAM_0030;
        break;
    }

    if((tx_frequency < 72000000) && (tx_band >= 0))
    {
        _board_state.selPortTX = RFE_PORT_3;
        _logger->log(Logger::LogLevelInfo, QString("TX port is now 70 MHz (J5)"));
        _low_band_tx = true;
    }
    else
    {
        if(_duplex_mode)
        {
            _board_state.selPortTX = RFE_PORT_2;
            _logger->log(Logger::LogLevelInfo, QString("TX port is now TX (J4)"));
        }
        else
        {
            _board_state.selPortTX = RFE_PORT_1;
            _logger->log(Logger::LogLevelInfo, QString("TX port is now TX/RX (J3)"));
        }
        _low_band_tx = false;
    }
    int res = RFE_ConfigureState(_lime_rfe, _board_state);
    if(res != RFE_SUCCESS)
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
}

void LimeRFEController::setAttenuator(int value)
{
    if(!_lime_rfe_inited)
        return;
    if((value < 0) || (value > 5))
    {
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE attenuator value %1 not supported").arg(value));
        return;
    }
    _board_state.attValue = value;
    int res = RFE_ConfigureState(_lime_rfe, _board_state);
    if(res != RFE_SUCCESS)
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
}

void LimeRFEController::setNotchFilter(bool enable)
{
    if(!_lime_rfe_inited)
        return;

    if(enable)
    {
        _board_state.notchOnOff = RFE_NOTCH_ON;
    }
    else
    {
        _board_state.notchOnOff = RFE_NOTCH_OFF;
    }
    int res = RFE_ConfigureState(_lime_rfe, _board_state);
    if(res != RFE_SUCCESS)
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
}
