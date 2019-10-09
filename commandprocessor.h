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

class CommandProcessor : public QObject
{
    Q_OBJECT
public:
    explicit CommandProcessor(Settings *settings, QObject *parent = nullptr);
    ~CommandProcessor();
    QStringList listAvailableCommands();
    bool validateCommand(QString message);
    QString runCommand(QString message);
    void buildCommandList();

signals:
    // FIXME: duplicates main window signals
    void startTransmission();
    void endTransmission();
    void sendText(QString text, bool repeat);
    void toggleRX(bool value);
    void toggleTX(bool value);
    void tuneFreq(qint64 center_freq);
    void tuneTxFreq(qint64 freq);
    void changeTxShift(qint64 center_freq);
    void fineTuneFreq(long center_freq);
    void toggleRxModemMode(int value);
    void toggleTxModemMode(int value);
    void setTxPower(int value);
    void setRxSensitivity(int value);
    void setSquelch(int value);
    void setVolume(int value);
    void setTxVolume(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void enableGUIConst(bool value);
    void enableGUIFFT(bool value);
    void enableRSSI(bool value);
    void enableDuplex(bool value);
    void startAutoTuneFreq(int step, int scan_direction);
    void stopAutoTuneFreq();
    //void startMemoryTune(RadioChannels* channels, int scan_direction);
    void stopMemoryTune();
    void usePTTForVOIP(bool value);
    void setVOIPForwarding(bool value);
    void setVox(bool value);
    void connectToServer(QString server, unsigned port);
    void disconnectFromServer();
    void changeChannel(int id);
    void setMute(bool value);
    void toggleRepeat(bool value);
    void stopRadio();
    void setCarrierOffset(qint64 offset);
    void newFFTSize(int);
    void setWaterfallFPS(int);
    void setSampleRate(int);
    void newFilterWidth(int);
    void enableAudioCompressor(bool value);
    void enableRelays(bool value);
    void calibrateRSSI(float value);

public slots:

private:
    struct command
    {
        command(QString what, int c_id) : action(what), id(c_id) {}
        QString action;
        int id;
    };

    Settings *_settings;
    QVector<command*> *_command_list;

    QStringList getCommand(QString message, int &command_index);
};

#endif // COMMANDPROCESSOR_H
