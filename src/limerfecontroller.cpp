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

    // try a test configuration
    int ret = RFE_Configure(_lime_rfe, RFE_CID_HAM_0145, RFE_CID_HAM_0145, RFE_PORT_1,
                            RFE_PORT_1, RFE_MODE_RX, RFE_NOTCH_OFF, 0, 0, 0);
    if (ret != 0)
    {
        _logger->log(Logger::LogLevelCritical,
                     QString("Could not configure LimeRFE device with default settings"));
        return;
    }
    // configure defaults
    int res = RFE_ConfigureState(_lime_rfe, _board_state);
    if(res != RFE_SUCCESS)
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(res));
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
                _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(res));
        }
        else
        {
            _board_state.mode = RFE_MODE_TX;
            int res = RFE_Mode(_lime_rfe, RFE_MODE_TX);
            if(res != RFE_SUCCESS)
                _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(res));
        }

    }
    else
    {
        _board_state.mode = RFE_MODE_RX;
        int res = RFE_Mode(_lime_rfe, RFE_MODE_RX);
        if(res != RFE_SUCCESS)
            _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(res));
    }
    _transmit_on = tx_on;
}

void LimeRFEController::setDuplex(bool duplex_mode)
{
    /// TODO
    if(!_lime_rfe_inited)
        return;
    if(duplex_mode && _duplex_mode)
        return;
    if(!duplex_mode && !_duplex_mode)
        return;
    if(duplex_mode && !_duplex_mode)
    {
        _board_state.selPortTX = RFE_PORT_2;
        int res = RFE_ConfigureState(_lime_rfe, _board_state);
        if(res != RFE_SUCCESS)
            _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(res));
    }
    if(!duplex_mode && _duplex_mode)
    {
        // duplex mode only > 144 MHz for now
        _board_state.selPortTX = RFE_PORT_1;
        int res = RFE_ConfigureState(_lime_rfe, _board_state);
        if(res != RFE_SUCCESS)
            _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(res));
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
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(res));
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
        _logger->log(Logger::LogLevelWarning, QString("LimeRFE failed configuration %1").arg(res));
}
