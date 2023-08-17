// Written by Adrian Musceac YO8RZZ , started August 2019.
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

#include "commandprocessor.h"

CommandProcessor::CommandProcessor(const Settings *settings, Logger *logger,
                                   RadioChannels *radio_channels, QObject *parent)
    : QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _command_list = new QVector<command*>;
    buildCommandList();
    _mode_list = new QVector<QString>;
    _gpredict_controller = new GPredictControl(settings, logger);
    _radio_channels = radio_channels;
    buildModeList(_mode_list);
}

CommandProcessor::~CommandProcessor()
{
    for(int i=0;i<_command_list->length();i++)
    {
        delete _command_list->at(i);
    }
    _command_list->clear();
    delete _command_list;
    _mode_list->clear();
    delete _mode_list;
    delete _gpredict_controller;
}


void CommandProcessor::parseMumbleMessage(QString message, int sender_id)
{
    if(sender_id < 0)
        return;
    if((message == "help") || (message == "?"))
    {
        QString response("Available commands:\n");
        QStringList available_commands = listAvailableCommands(true);
        for(int i=0;i<available_commands.length();i++)
        {
            response.append(available_commands.at(i));
        }
        emit newCommandMessage(response,sender_id);
        return;
    }

    if(!validateCommand(message))
    {
        QString response("Command not recognized\n");
        emit newCommandMessage(response,sender_id);
        return;
    }

    QString result = runCommand(message, true);
    if(result != "")
    {
        QString response;
        response.append(result);
        response.append("\n");
        emit newCommandMessage(response,sender_id);
        return;
    }
    else
    {
        QString response("Command not processed\n");
        emit newCommandMessage(response,sender_id);
        return;
    }
}


QStringList CommandProcessor::listAvailableCommands(bool mumble_text)
{
    QStringList list;
    if(!mumble_text)
    {
        for(int i=0;i<_command_list->size();i++)
        {
            list.append("\e[34m" + _command_list->at(i)->action + "\e[0m\e[32m"
                        + QString(" (%1 parameters): %2").arg(
                            _command_list->at(i)->params).arg(_command_list->at(i)->help_msg)
                        + "\e[0m\n");
        }
        list.append("\e[34mhelp\e[0m\n");
        list.append("\e[34m?\e[0m\n");
        list.append("\e[34mexit\e[0m\n");
        list.append("\e[34mquit\e[0m\n");
    }
    else
    {
        for(int i=0;i<_command_list->size();i++)
        {
            list.append(_command_list->at(i)->action
                        + QString(" (%1 parameters): %2").arg(
                            _command_list->at(i)->params).arg(_command_list->at(i)->help_msg)
                        + "<br/>\n");
        }
        list.append("help<br/>\n");
        list.append("?<br/>\n");
        list.append("exit<br/>\n");
        list.append("quit<br/>\n");
    }
    return list;
}

QString CommandProcessor::processGPredictMessages(QString message)
{
    int action = GPredictControl::RadioAction::NoAction;
    qint64 rx_freq = 0;
    qint64 tx_freq = 0;
    qint64 rx_freq_delta = 0;
    qint64 tx_freq_delta = 0;
    QString ret_msg = _gpredict_controller->processMessages(message, action, rx_freq, tx_freq,
                                                 rx_freq_delta, tx_freq_delta);
    switch(action)
    {
    case GPredictControl::RadioAction::TuneRX:
        emit tuneFreq(rx_freq);
        break;
    case GPredictControl::RadioAction::TuneTX:
        emit setShiftFromTxFreq(tx_freq);
        break;
    case GPredictControl::RadioAction::OffsetRX:
        emit tuneDopplerRxFreq(rx_freq_delta);
        break;
    case GPredictControl::RadioAction::OffsetTX:
        emit tuneDopplerTxFreq(tx_freq_delta);
        break;
    case GPredictControl::RadioAction::NoAction:
        break;
    default:
        break;
    }
    return ret_msg;
}

void CommandProcessor::endGPredictControl()
{
    emit resetTxCarrierOffset();
}

QStringList CommandProcessor::getCommand(QString message, int &command_index)
{
    command_index = -1;
    message = message.trimmed();
    QStringList tokens = message.split(" ");
    if((tokens.length() > 4) || (tokens.length() < 1))
    {
        QStringList none("");
        return none;
    }

    QString action = tokens.at(0);

    for(int i=0; i< _command_list->length();i++)
    {
        if((_command_list->at(i)->action == action)
                && (_command_list->at(i)->params == (tokens.length() - 1)))
        {
            command_index = i;
            break;
        }
    }
    return tokens;
}

bool CommandProcessor::validateCommand(QString message)
{
    QRegularExpression re("^[a-zA-Z0-9_]+[\\sa-zA-Z0-9_.\\-]*\\r?\\n?$");
    QRegularExpressionValidator validator(re, 0);

    int pos = 0;
    if(QValidator::Acceptable != validator.validate(message, pos))
    {
        // regex match failed
        return false;
    }
    int command_index = -1;
    getCommand(message, command_index);

    if(command_index < 0)
    {
        // command not found
        return false;
    }
    return true;
}

QString CommandProcessor::runCommand(QString message, bool mumble)
{
    int command_index;
    QString response;
    QStringList tokens = getCommand(message, command_index);
    QString param1, param2, param3;

    if(tokens.size() > 1)
    {
        param1 = tokens.at(1);
    }
    if(tokens.size() > 2)
    {
        param2 = tokens.at(2);
    }
    if(tokens.size() > 3)
    {
        param3 = tokens.at(3);
    }

    /// Actual command processing
    ///
    bool success;
    success = processStatusCommands(command_index, response);
    if(!success)
    {
        if(mumble)
            return response;
        else
            return "\e[31m" + response + "\e[0m\n";
    }
    success = processActionCommands(command_index, response, param1, param2, param3);
    if(!success)
    {
        if(mumble)
            return response;
        else
            return "\e[31m" + response + "\e[0m\n";
    }

    if(response.length() < 1)
    {
        if(mumble)
            return "Command not implemented\n";
        else
            return "\e[31mCommand not implemented\e[0m\n";
    }
    if(mumble)
        return response;
    else
        return "\e[32m" + response + "\e[0m\n";
}

bool CommandProcessor::processStatusCommands(int command_index, QString &response)
{
    bool success = true;
    switch (command_index) {
    case 0:
        if(_settings->rx_inited)
            response.append("RX status is active.");
        else
            response.append("RX status is inactive.");
        break;
    case 1:
        if(_settings->tx_inited)
            response.append("TX status is active.");
        else
            response.append("TX status is inactive.");
        break;
    case 2:
        if(_settings->tx_started)
            response.append("Currently transmitting.");
        else
            response.append("Not transmitting.");
        break;
    case 3:
        response.append(QString("Current RX mode is: %1 - %2.").arg(_settings->rx_mode).arg(
                                                                    _mode_list->at(_settings->rx_mode)));
        break;
    case 4:
        response.append(QString("Current TX mode is: %1 - %2.").arg(_settings->tx_mode).arg(
                                                                    _mode_list->at(_settings->tx_mode)));
        break;
    case 5:
        response.append(QString("Current RX CTCSS tone is %1.").arg(_settings->rx_ctcss));
        break;
    case 6:
        response.append(QString("Current TX CTCSS tone is %1.").arg(_settings->tx_ctcss));
        break;
    case 7:
        response.append(QString("Current RX volume is %1.").arg(_settings->rx_volume));
        break;
    case 8:
        response.append(QString("Current TX volume is %1.").arg(_settings->tx_volume));
        break;
    case 9:
        response.append(QString("Current Squelch value is %1.").arg(_settings->squelch));
        break;
    case 10:
        response.append(QString("Current RX gain is %1.").arg(_settings->rx_sensitivity));
        break;
    case 11:
        response.append(QString("Current TX gain value is %1.").arg(_settings->tx_power));
        break;
    case 12:
        response.append(QString("Current RSSI value is %1.").arg(_settings->rssi));
        break;
    case 13:
        if(_settings->voip_connected)
        {
            response.append(
                QString("Connected to VOIP server: %1; Channel number: %2.").arg(
                        _settings->voip_server).arg(_settings->current_voip_channel));
        }
        else
        {
            response.append("Not connected to VOIP network");
        }
        break;
    case 14:
        if(_settings->voip_forwarding)
            response.append(QString("Radio is forwarded to VOIP network."));
        else
            response.append(QString("Radio is not forwarded to VOIP network."));
        break;
    case 15:
        if(_settings->vox_enabled)
            response.append(QString("VOX is on."));
        else
            response.append(QString("VOX is off."));
        break;
    case 16:
        if(_settings->repeater_enabled)
            response.append(QString("Repeater is enabled."));
        else
            response.append(QString("Repeater is disabled."));
        break;
    case 17:
        if(_settings->enable_duplex)
            response.append(QString("Duplex is enabled."));
        else
            response.append(QString("Duplex is disabled."));
        break;
    case 54:
        for(int i=0;i<_mode_list->size();i++)
            response.append(QString::number(i) + ": " + _mode_list->at(i) + "\n");
        break;
    case 55:
        if(_settings->recording_audio)
            response.append(QString("Audio recording is enabled."));
        else
            response.append(QString("Audio recording is disabled."));
        break;
    case 57:
        response.append(QString("Current VOX level is %1.").arg(_settings->vox_level));
        break;
    case 59:
        response.append(QString("Current VOIP bitrate is %1.").arg(_settings->voip_bitrate));
        break;
    case 61:
        if(_settings->mute_forwarded_audio)
            response.append(QString("Local audio muting while forwarding is enabled."));
        else
            response.append(QString("Local audio muting while forwarding is disabled."));
        break;
    case 63:
        if(_settings->tx_band_limits)
            response.append(QString("TX band limits are enabled."));
        else
            response.append(QString("TX band limits are disabled."));
        break;
    case 65:
    {
        QVector<radiochannel*> *channels = _radio_channels->getChannels();
        for(int i=0;i<channels->size();i++)
        {
            radiochannel *channel = channels->at(i);
            response.append(QString::number(i) + ": " + QString::fromStdString(channel->name) +
                            " | Freq: " + QString::number(channel->rx_frequency) + " | RX mode: " +
                            _mode_list->at(channel->rx_mode) + " | TX mode: " +
                            _mode_list->at(channel->tx_mode) +
                            " | Shift: " + QString::number(channel->tx_shift) + "|" +"\n");
        }
        break;
    }
    case 68:
        response.append(QString("Current AGC attack is %1.").arg(_settings->agc_attack));
        break;
    case 69:
        response.append(QString("Current AGC decay is %1.").arg(_settings->agc_decay));
        break;
    case 72:
        response.append(QString("Current RX frequency is %1.").arg(_settings->rx_frequency +
                                                                   _settings->demod_offset + _settings->lnb_lo_freq));
        break;
    case 73:
        response.append(QString("Current TX frequency is %1.").arg(_settings->rx_frequency +
                                                                   _settings->demod_offset +
                                                                   _settings->lnb_lo_freq + _settings->tx_shift));
        break;
    case 74:
        if(_settings->udp_enabled)
            response.append("UDP streaming is enabled.");
        else
            response.append("UDP streaming is disabled.");
        break;
    case 76:
        response.append(QString("Current VOIP volume is %1.").arg(_settings->voip_volume));
        break;

    default:
        break;
    }
    return success;
}

bool CommandProcessor::processActionCommands(int command_index, QString &response,
                                             QString param1, QString param2, QString param3)
{
    Q_UNUSED(param3);
    bool success = true;
    switch (command_index) {
    case 18:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            QString on = (set ==0) ? "off" : "on";
            response = QString("Turning %1 receiver").arg(on);
            emit toggleRX((bool)set);
        }
        break;
    }
    case 19:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            QString on = (set ==0) ? "off" : "on";
            response = QString("Turning %1 transmitter").arg(on);
            emit toggleTX((bool)set);
        }
        break;
    }
    case 20:
    {
        int set = param1.toInt();
        if((set < 0) || (set >= _mode_list->size()))
        {
            response = "Operating mode not found";
            success = false;
        }
        else
        {
            response = QString("Setting receiver mode to %1").arg(_mode_list->at(set));
            emit toggleRxModemMode(set);
        }
        break;
    }
    case 21:
    {
        int set = param1.toInt();
        if(set < 0 || set >= _mode_list->size())
        {
            response = "Operating mode not found";
            success = false;
        }
        else
        {
            response = QString("Setting transmitter mode to %1").arg(_mode_list->at(set));
            emit toggleTxModemMode(set);
        }
        break;
    }
    case 22:
    {
        float set = param1.toFloat();
        if((set < 0.0) || (set > 250.0))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting RX CTCSS to %1").arg(set);
            emit setRxCTCSS(set);
        }
        break;
    }
    case 23:
    {
        float set = param1.toFloat();
        if((set < 0.0) || (set > 250.0))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting TX CTCSS to %1").arg(set);
            emit setTxCTCSS(set);
        }
        break;
    }
    case 24:
    {
        int set = param1.toInt();
        if((set < -150) || (set > 10))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting squelch value to %1").arg(set);
            emit setSquelch(set);
        }
        break;
    }
    case 25:
    {
        int set = param1.toInt();
        if((set < 0) || (set > 100))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting RX volume value to %1").arg(set);
            emit setVolume(set);
        }
        break;
    }
    case 26:
    {
        int set = param1.toInt();
        if((set < 0) || (set > 100))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting TX volume value to %1").arg(set);
            emit setTxVolume(set);
        }
        break;
    }
    case 27:
    {
        int set = param1.toInt();
        if((set < 0) || (set > 100))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting RX gain value to %1").arg(set);
            emit setRxSensitivity(set);
        }
        break;
    }
    case 28:
    {
        int set = param1.toInt();
        if((set < 0) || (set > 100))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting TX gain value to %1").arg(set);
            std::string gain_stage = "";
            emit setTxPower(set, gain_stage);
        }
        break;
    }
    case 29:
    {
        int set = param1.toLong();
        if(set < 1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            /// rx frequency is the source center frequency
            int64_t rx_frequency = set - _settings->demod_offset - _settings->lnb_lo_freq;
            /// tx_frequency is the actual frequency
            int64_t tx_frequency = set - _settings->lnb_lo_freq;
            emit tuneFreq(rx_frequency);
            emit tuneTxFreq(tx_frequency);
            response = QString("Tuning receiver to %L1 Hz").arg(set);
        }
        break;
    }
    case 30:
    {
        response = QString("Use command tunerx to tune the transmitter along with the receiver. TX shift will be preserved.");
        success = false;
        break;
    }
    case 31:
    {
        int set = param1.toLong();
        if(set < -_settings->rx_sample_rate/2 || set > _settings->rx_sample_rate/2)
        {
            response = "Carrier offset must be less than rx_sample_rate/2";
            success = false;
        }
        {
            response = QString("Setting demodulator offset to to %L1 Hz").arg(set);
            emit setCarrierOffset(set);
        }
        break;
    }
    case 32:
    {
        int set = param1.toLong();
        response = QString("Setting TX shift to to %L1 Hz").arg(set);
        emit changeTxShift(set);
        break;
    }
    case 33:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting duplex to %1").arg(set);
            emit enableDuplex((bool)set);
        }
        break;
    }
    case 34:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting radio forwarding to %1").arg(set);
            emit setVOIPForwarding((bool)set);
        }
        break;
    }
    case 35:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else if(!_settings->enable_duplex)
        {
            response = "Repeater mode can only function in duplex mode";
            success = false;
        }
        else
        {
            response = QString("Setting repeater to %1").arg(set);
            emit toggleRepeat((bool)set);
        }
        break;
    }
    case 36:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting VOX to %1").arg(set);
            emit setVox((bool)set);
        }
        break;
    }
    case 37:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting PTT for VOIP to %1").arg(set);
            emit usePTTForVOIP((bool)set);
        }
        break;
    }
    case 38:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting audio compressor to %1").arg(set);
            emit enableAudioCompressor((bool)set);
        }
        break;
    }
    case 39:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting relay controls to %1").arg(set);
            emit enableRelays((bool)set);
        }
        break;
    }
    case 40:
    {
        int set = param1.toInt();
        if((set < -180) || (set > 30))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting RSSI calibration to %1 dBm").arg(set);
            emit calibrateRSSI((float)set);
        }
        break;
    }
    case 41:
    {
        int set = param1.toInt();
        if((set < 1) || (set > 30))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting RX sample rate to %1 Msps").arg(set);
            emit setSampleRate(set * 1000000);
        }
        break;
    }
    case 42:
    {
        if(_settings->rssi > 99.0f)
        {
            response = "Could not set auto squelch";
            success = false;
        }
        else
        {
            int squelch = (int)_settings->rssi +
                    (abs(_settings->rssi_calibration_value) - 80) + 50;
            response = QString("Setting squelch automatically to %1").arg(squelch);
            emit setSquelch(squelch);
        }
        break;
    }
    case 43:
    {
        int set = param1.toInt();
        if((set < 800) || (set > 100000))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting filter width to %1 Hertz").arg(set);
            emit newFilterWidth(set);
        }
        break;
    }
    case 44:
    {
        response = "Starting transmission";
        emit startTransmission();
        break;
    }
    case 45:
    {
        response = "Stopping transmission";
        emit endTransmission();
        break;
    }
    case 46:
    {
        QString host = param1;
        int port = param2.toInt();
        if((port < 0) || (port > 65535))
        {
            response = "Port parameter value is not supported";
            success = false;
        }
        else if(_settings->voip_connected)
        {
            response = "Already connected";
            success = false;
        }
        else
        {
            response = QString("Connecting to Mumble server %1 at port %2").arg(host).arg(port);
            emit connectToServer(host,port);
        }
        break;
    }
    case 47:
    {
        if(!_settings->voip_connected)
        {
            response = "Not connected";
            success = false;
        }
        else
        {
            response = "Disconnecting from Mumble server";
            emit disconnectFromServer();
        }
        break;
    }
    case 48:
    {
        int set = param1.toInt();
        if(!_settings->voip_connected)
        {
            response = "Not connected";
            success = false;
        }
        if((set < 0) || (set > 65536))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Changing channel to %1").arg(set);
            emit changeChannel(set);
        }
        break;
    }
    case 49:
    {
        break;
        if(!_settings->voip_connected)
        {
            response = "Not connected";
            success = false;
        }
        if(param1.size() > 1024)
        {
            response = "Text too long, 1024 characters at most";
            success = false;
        }
        else
        {
            response = QString("Sending text message to channel %1").arg(
                        _settings->current_voip_channel);
            emit newMumbleMessage(param1);
        }
        break;
    }
    case 50:
    {
        if(!_settings->voip_connected)
        {
            response = "Not connected";
            success = false;
        }
        int set = param1.toInt();
        if((set != 1) && (set != 0))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting Mumble mute to %1").arg(set);
            emit setMute((bool)set);
        }
        break;
    }
    case 51:
    {
        break;
        if(!_settings->tx_inited)
        {
            response = "TX is not started";
            success = false;
        }
        if(param1.size() > 4096)
        {
            response = "Text too long, 4096 characters at most";
            success = false;
        }
        else
        {
            response = QString("Sending text message to channel %1").arg(
                        _settings->current_voip_channel);
            emit sendText(param1, false);
        }
        break;
    }
    case 52:
    {
        response = QString(
            "Starting transceiver with RX frequency: %1, TX frequency %2, TX shift %3").arg(
            _settings->rx_frequency).arg(_settings->rx_frequency).arg(_settings->tx_shift);
        emit enableGUIFFT(false);
        emit enableGUIConst(false);
        emit enableRSSI(true);
        emit setWaterfallFPS(10);
        emit toggleRX(true);
        emit toggleTX(true);
        break;
    }
    case 53:
    {
        response = QString("Stopping transceiver");
        emit endTransmission(); // just in case
        emit toggleRX(false);
        emit toggleTX(false);
        break;
    }
    case 56:
    {
        int set = param1.toInt();
        if((set != 1) && (set != 0))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting Audio recording to %1").arg(set);
            emit setAudioRecord((bool)set);
        }
        break;
    }
    case 58:
    {
        int set = param1.toInt();
        if((set < 0) || (set > 100))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting VOX level to %1").arg(set);
            emit setVoxLevel(set);
        }
        break;
    }
    case 60:
    {
        int set = param1.toInt();
        if((set < 9400) || (set > 50000))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting VOIP bitrate to %1").arg(set);
            emit setVoipBitrate(set);
        }
        break;
    }
    case 62:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting local audio mute while forwarding to %1").arg(set);
            emit setMuteForwardedAudio((bool)set);
        }
        break;
    }
    case 64:
    {
        int set = param1.toInt();
        if(set != 0 && set !=1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting TX band limits to %1").arg(set);
            emit setTxLimits((bool)set);
        }
        break;
    }
    case 66:
    {
        QVector<radiochannel*> *channels = _radio_channels->getChannels();
        int set = param1.toInt();
        if(set < 0 || set >= channels->size())
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting Radio channel to %1").arg(set);
            emit tuneMemoryChannel(channels->at(set));
        }
        break;
    }
    case 67:
    {
        response = QString("Will shutdown now");
        emit stopRadio();
        break;
    }
    case 70:
    {
        int set = param1.toInt();
        if((set < -25) || (set > 25))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting AGC attack level to %1").arg(set);
            emit setAgcAttack(set);
        }
        break;
    }
    case 71:
    {
        int set = param1.toInt();
        if((set < -25) || (set > 25))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting AGC decay level to %1").arg(set);
            emit setAgcDecay(set);
        }
        break;
    }
    case 75:
    {
        int set = param1.toInt();
        if(set != 0 && set != 1)
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting UDP streaming to %1").arg(set);
            emit setUDPEnabled((bool)set);
        }
        break;
    }
    case 77:
    {
        int set = param1.toInt();
        if((set < 0) || (set > 100))
        {
            response = "Parameter value is not supported";
            success = false;
        }
        else
        {
            response = QString("Setting VOIP volume value to %1").arg(set);
            emit setVoipVolume(set);
        }
        break;
    }

    default:
        break;
    }
    return success;
}

void CommandProcessor::buildCommandList()
{
    /// status commands
    _command_list->append(new command("rxstatus", 0, "Status of receiver (started or not)"));
    _command_list->append(new command("txstatus", 0, "Status of transmitter (started or not)"));
    _command_list->append(new command("txactive", 0, "See if the radio is on the air"));
    _command_list->append(new command("rxmode", 0, "Get RX operating mode"));
    _command_list->append(new command("txmode", 0, "Get TX operating mode"));
    _command_list->append(new command("rxctcss", 0, "Get RX CTCSS"));
    _command_list->append(new command("txctcss", 0, "Get TX CTCSS"));
    _command_list->append(new command("rxvolume", 0, "Get RX volume value"));
    _command_list->append(new command("txvolume", 0, "Get TX volume value"));
    _command_list->append(new command("squelch", 0, "Get squelch value"));
    _command_list->append(new command("rxgain", 0, "Get RX gain value"));
    _command_list->append(new command("txgain", 0, "Get TX gain value"));
    _command_list->append(new command("rssi", 0, "Get current RSSI value"));
    _command_list->append(new command("voipstatus", 0, "Get VOIP status"));
    _command_list->append(new command("forwardingstatus", 0, "Get radio forwarding status"));
    _command_list->append(new command("voxstatus", 0, "Get VOX status"));
    _command_list->append(new command("repeaterstatus", 0, "Get repeater status"));
    _command_list->append(new command("duplexstatus", 0, "Get duplex status"));

    /// action commands
    _command_list->append(new command("setrx", 1, "Start/stop receiver, 1 enabled, 0 disabled"));
    _command_list->append(new command("settx", 1, "Start/stop transmitter, 1 enabled, 0 disabled"));
    _command_list->append(new command("setrxmode", 1, "Set RX mode (integer number, see list_modes)"));
    _command_list->append(new command("settxmode", 1, "Set TX mode (integer number, see list_modes)"));
    _command_list->append(new command("setrxctcss", 1, "Set RX CTCSS (floating point number, 0.0 to 200.0)"));
    _command_list->append(new command("settxctcss", 1, "Set TX CTCSS (floating point number, 0.0 to 200.0)"));
    _command_list->append(new command("setsquelch", 1, "Set squelch (integer number, -150 to 10)"));
    _command_list->append(new command("setrxvolume", 1, "Set RX volume (integer number, 0 to 100)"));
    _command_list->append(new command("settxvolume", 1, "Set TX volume (integer number, 0 to 100)"));
    _command_list->append(new command("setrxgain", 1, "Set RX gain (integer number, 0 to 99)"));
    _command_list->append(new command("settxgain", 1, "Set TX gain (integer number, 0 to 99)"));
    _command_list->append(new command("tunerx", 1, "Tune RX frequency, (integer value in Hertz)"));
    _command_list->append(new command("tunetx", 1, "Tune TX frequency, (integer value in Hertz)"));
    _command_list->append(new command("setoffset", 1, "Set demodulator offset, (integer value in Hertz)"));
    _command_list->append(new command("setshift", 1, "Set TX shift, (integer value in Hertz)"));
    _command_list->append(new command("setduplex", 1, "Set duplex mode, (1 enabled, 0 disabled)"));
    _command_list->append(new command("setforwarding", 1, "Set radio forwarding mode, (1 enabled, 0 disabled)"));
    _command_list->append(new command("setrepeater", 1, "Set repeater mode, (1 enabled, 0 disabled)"));
    _command_list->append(new command("setvox", 1, "Set vox mode, (1 enabled, 0 disabled)"));
    _command_list->append(new command("setpttvoip", 1, "Use PTT for VOIP, (1 enabled, 0 disabled)"));
    _command_list->append(new command("setcompressor", 1, "Enable audio compressor, (1 enabled, 0 disabled)"));
    _command_list->append(new command("setrelays", 1, "Enable relay control, (1 enabled, 0 disabled)"));
    _command_list->append(new command("setrssicalibration", 1, "Set RSSI calibration, (integer value in dBm)"));
    _command_list->append(new command("setrxsamprate", 1, "Set RX sample rate, (integer value in Msps)"));
    _command_list->append(new command("autosquelch", 0, "Set autosquelch"));
    _command_list->append(new command("setfilterwidth", 1, "Set filter width (analog only), (integer value in Hz)"));
    _command_list->append(new command("ptt_on", 0, "Transmit"));
    _command_list->append(new command("ptt_off", 0, "Stop transmitting"));
    _command_list->append(new command("connectserver", 2, "Connect to Mumble server, (string value hostname, integer value port)"));
    _command_list->append(new command("disconnectserver", 0, "Disconnect from Mumble server"));
    _command_list->append(new command("changechannel", 1, "Change channel to channel number (integer channel number)"));
    _command_list->append(new command("mumblemsg", 1, "Send Mumble message, (string value text)"));
    _command_list->append(new command("mutemumble", 1, "Mute Mumble connection, (1 enabled, 0 disabled)"));
    _command_list->append(new command("textmsg", 1, "Send radio text message, (string value text)"));
    _command_list->append(new command("start_trx", 0, "Convenience function, requires everything to be preconfigured"));
    _command_list->append(new command("stop_trx", 0, "Convenience function, requires everything to be preconfigured"));

    /// I'm sorry
    _command_list->append(new command("list_modes", 0, "List operating modes"));
    _command_list->append(new command("recordstatus", 0, "Status of audio recorder"));
    _command_list->append(new command("setaudiorecorder", 1, "Toggle audio recording, (1 enabled, 0 disabled)"));
    _command_list->append(new command("voxlevel", 0, "Get VOX level"));
    _command_list->append(new command("setvoxlevel", 1, "Set VOX level (integer value level between 0 and 100)"));
    _command_list->append(new command("voipbitrate", 0, "Get VOIP bitrate"));
    _command_list->append(new command("setvoipbitrate", 1, "Set VOIP bitrate (bits/sec"));
    _command_list->append(new command("muteforwarding", 0, "Get local mute status of VOIP forwarded radio"));
    _command_list->append(new command("setmuteforwarding", 1, "Toggle local mute status of VOIP forwarded radio, (1 enabled, 0 disabled)"));
    _command_list->append(new command("gettxlimits", 0, "Get status of TX band limiter"));
    _command_list->append(new command("settxlimits", 1, "Toggle TX band limits, (1 enabled, 0 disabled)"));
    _command_list->append(new command("listradiochan", 0, "List memory channels"));
    _command_list->append(new command("setradiochan", 1, "Set radio channel (integer value)"));
    _command_list->append(new command("shutdown", 0, "Shutdown and exit"));
    _command_list->append(new command("agcattack", 0, "Get AGC attack value"));
    _command_list->append(new command("agcdecay", 0, "Get AGC decay value"));
    _command_list->append(new command("setagcattack", 1, "Set AGC attack value"));
    _command_list->append(new command("setagcdecay", 1, "Set AGC decay value"));
    _command_list->append(new command("rxfreq", 0, "Get current RX frequency"));
    _command_list->append(new command("txfreq", 0, "Get current TX frequency"));
    _command_list->append(new command("udpstatus", 0, "Get UDP audio forwarding status"));
    _command_list->append(new command("setudpenabled", 1, "Set UDP streaming mode, (1 enabled, 0 disabled)"));
    _command_list->append(new command("voipvolume", 0, "Get VOIP volume value"));
    _command_list->append(new command("setvoipvolume", 1, "Set VOIP volume value, (integer value level between 0 and 100)"));
}
