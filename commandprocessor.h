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


#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QRegularExpressionValidator>
#include <QVector>
#include "settings.h"
#include "logger.h"
#include "ext/utils.h"

class CommandProcessor : public QObject
{
    Q_OBJECT
public:
    explicit CommandProcessor(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~CommandProcessor();
    QStringList listAvailableCommands();
    bool validateCommand(QString message);
    QString runCommand(QString message, bool mumble=false);
    void buildCommandList();

signals:
    // FIXME: duplicates main window signals

    /// general settings
    void toggleRX(bool value);
    void toggleTX(bool value);
    void toggleRxModemMode(int value);
    void toggleTxModemMode(int value);
    void setTxPower(int value);
    void setRxSensitivity(int value);
    void setSquelch(int value);
    void setVolume(int value);
    void setTxVolume(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void usePTTForVOIP(bool value);
    void setVOIPForwarding(bool value);
    void setVox(bool value);
    void toggleRepeat(bool value);
    void enableAudioCompressor(bool value);
    void enableRelays(bool value);
    void enableDuplex(bool value);
    void calibrateRSSI(float value);
    void setAgcAttack(float value);
    void setAgcDecay(float value);
    void setSampleRate(int);
    void newFilterWidth(int);

    /// Tuning the radio
    void setCarrierOffset(qint64 offset);
    void tuneFreq(qint64 center_freq);
    void tuneTxFreq(qint64 freq);
    void changeTxShift(qint64 center_freq);
    void fineTuneFreq(long center_freq);

    /// Transmit
    void startTransmission();
    void endTransmission();
    void sendText(QString text, bool repeat);


    /// Scannning
    void startAutoTuneFreq(int step, int scan_direction);
    void stopAutoTuneFreq();
    void startMemoryTune(int scan_direction);
    void stopMemoryTune();

    /// VOIP
    void connectToServer(QString server, unsigned port);
    void disconnectFromServer();
    void changeChannel(int id);
    void setMute(bool value);
    void newMumbleMessage(QString text);
    void newCommandMessage(QString text, int to_id);
    void toggleSelfDeaf(bool deaf);
    void toggleSelfMute(bool mute);

    //void stopRadio();

    /// GUI
    void enableGUIFFT(bool value);
    void enableRSSI(bool value);
    void enableGUIConst(bool value);
    void newFFTSize(int);
    void setWaterfallFPS(int);


public slots:
    void parseMumbleMessage(QString message, int sender_id);

private:
    struct command
    {
        command(QString what, int n_param=0, QString help_msg="") : action(what), params(n_param),
            help_msg(help_msg) {}
        QString action;
        int params;
        QString help_msg;
    };

    const Settings *_settings;
    Logger *_logger;
    QVector<command*> *_command_list;
    QVector<QString> *_mode_list;

    QStringList getCommand(QString message, int &command_index);
    bool processStatusCommands(int command_index, QString &response);
    bool processActionCommands(int command_index, QString &response,
                               QString param1, QString param2, QString param3);
};

#endif // COMMANDPROCESSOR_H
