#include "limerfecontroller.h"

LimeRFEController::LimeRFEController(const Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{

    _logger = logger;
    _settings = settings;
    _lime_rfe_inited = false;
    _duplex_mode = false;
    _transmit_on = false;
    _current_rx_band = 12;
    _current_tx_band = 12;
    _limits = new Limits();
    // board defaults
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
    if(_duplex_mode)
    {
        _logger->log(Logger::LogLevelWarning, "LimeRFE is in duplex mode and switches cannot be toggled");
        return;
    }
    if(tx_on)
    {
        if(_duplex_mode)
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
        _board_state.selPortTX = RFE_PORT_2;
        int res = RFE_ConfigureState(_lime_rfe, _board_state);
        if(res != RFE_SUCCESS)
            _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
    }
    if(!duplex_mode && _duplex_mode)
    {
        // duplex mode only > 144 MHz for now
        _board_state.selPortTX = RFE_PORT_1;
        int res = RFE_ConfigureState(_lime_rfe, _board_state);
        if(res != RFE_SUCCESS)
            _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
    }
    _duplex_mode = duplex_mode;
}

void LimeRFEController::setRXBand(int64_t rx_frequency)
{
    if(!_lime_rfe_inited)
        return;

    int rx_band = _limits->getBand(rx_frequency);
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
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        _board_state.channelIDRX = RFE_CID_HAM_0030;
        break;
    case 10:
        _board_state.channelIDRX = RFE_CID_HAM_0070;
        break;
    case 11:
        _board_state.channelIDRX = RFE_CID_HAM_0070;
        break;
    case 12:
        _board_state.channelIDRX = RFE_CID_HAM_0145;
        break;
    case 13:
        _board_state.channelIDRX = RFE_CID_HAM_0435;
        break;
    case 14:
        _board_state.channelIDRX = RFE_CID_HAM_1280;
        break;
    case 15:
        _board_state.channelIDRX = RFE_CID_HAM_2400;
        break;
    case 16:
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

    int tx_band = _limits->getBand(tx_frequency);
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
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        _board_state.channelIDTX = RFE_CID_HAM_0030;
        break;
    case 10:
        _board_state.channelIDTX = RFE_CID_HAM_0070;
        break;
    case 11:
        _board_state.channelIDTX = RFE_CID_HAM_0070;
        break;
    case 12:
        _board_state.channelIDTX = RFE_CID_HAM_0145;
        break;
    case 13:
        _board_state.channelIDTX = RFE_CID_HAM_0435;
        break;
    case 14:
        _board_state.channelIDTX = RFE_CID_HAM_1280;
        break;
    case 15:
        _board_state.channelIDTX = RFE_CID_HAM_2400;
        break;
    case 16:
        _board_state.channelIDTX = RFE_CID_HAM_3500;
        break;
    default:
        _board_state.channelIDTX = RFE_CID_HAM_0030;
        break;
    }
    if(tx_frequency < 72000000)
    {
        if(_duplex_mode)
            _board_state.selPortTX = RFE_PORT_2;
        else
            _board_state.selPortTX = RFE_PORT_3;
    }
    else
    {
        if(_duplex_mode)
            _board_state.selPortTX = RFE_PORT_2;
        else
            _board_state.selPortTX = RFE_PORT_1;
    }
    int res = RFE_ConfigureState(_lime_rfe, _board_state);
    if(res != RFE_SUCCESS)
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(getError(res)));
}
