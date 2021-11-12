#include "limerfecontroller.h"

LimeRFEController::LimeRFEController(const Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{

    _logger = logger;
    _settings = settings;
    _lime_rfe_inited = false;
}

LimeRFEController::~LimeRFEController()
{
    deinit();
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
    int ret = RFE_Configure(_lime_rfe, RFE_CID_HAM_0145, RFE_CID_HAM_0145, RFE_PORT_1, RFE_PORT_1, RFE_MODE_RX, RFE_NOTCH_OFF, 0, 0, 0);
    if (ret != 0)
    {
        _logger->log(Logger::LogLevelCritical,
                     QString("Could not configure LimeRFE device with default settings"));
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
