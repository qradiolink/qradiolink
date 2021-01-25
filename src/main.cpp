// Written by Adrian Musceac YO8RZZ , started March 2016.
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

#include <QApplication>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMetaType>
#include <QtGlobal>
#include <string>
#include "src/mainwindow.h"
#include "src/mumbleclient.h"
#include "src/audio/audiowriter.h"
#include "src/audio/audioreader.h"
#include "src/mumblechannel.h"
#include "src/radiochannel.h"
#include "src/radiocontroller.h"
#include "src/telnetserver.h"
#include "src/logger.h"

void connectIndependentSignals(AudioWriter *audiowriter, AudioReader *audioreader,
                               RadioController *radio_op, MumbleClient *mumbleclient, TelnetServer *telnet_server);
void connectGuiSignals(TelnetServer *telnet_server, AudioWriter *audiowriter,
                       AudioReader *audioreader, MainWindow *w, MumbleClient *mumbleclient,
                       RadioController *radio_op, Logger *logger);
void connectCommandSignals(TelnetServer *telnet_server, MumbleClient *mumbleclient,
                       RadioController *radio_op);
class Station;

int main(int argc, char *argv[])
{

    typedef QVector<Station*> StationList;
    qRegisterMetaType<StationList>("StationList");
    typedef QVector<MumbleChannel*> ChannelList;
    qRegisterMetaType<ChannelList>("ChannelList");
    typedef std::vector<std::complex<float>> complex_vector;
    qRegisterMetaType<complex_vector>("complex_vector");
    typedef QMap<std::string,QVector<int>> gain_vector;
    qRegisterMetaType<gain_vector>("gain_vector");
    qRegisterMetaType<std::string>("std::string");


    QApplication a(argc, argv);
    QStringList arguments = QCoreApplication::arguments();
    bool headless = false;

    Logger *logger = new Logger;
    if((arguments.length() > 1) && (arguments.indexOf("--headless") != -1))
    {
        logger->set_console_log(false);
        headless = true;
    }

    /// Init main logic
    ///
    logger->log(Logger::LogLevelInfo, "Starting qradiolink");
    Settings *settings = new Settings(logger);
    settings->readConfig();
    RadioChannels *radio_channels = new RadioChannels(logger);
    radio_channels->readConfig();
    MumbleClient *mumbleclient = new MumbleClient(settings, logger);
    RadioController *radio_op = new RadioController(settings, logger, radio_channels);
    AudioWriter *audiowriter = new AudioWriter(settings, logger);
    AudioReader *audioreader = new AudioReader(settings, logger);
    TelnetServer *telnet_server = new TelnetServer(settings, logger, radio_channels);


    /// Init threads
    ///
    QThread *t1 = new QThread;
    t1->setObjectName("radioop");
    radio_op->moveToThread(t1);
    QObject::connect(t1, SIGNAL(started()), radio_op, SLOT(run()));
    QObject::connect(radio_op, SIGNAL(finished()), t1, SLOT(quit()));
    QObject::connect(radio_op, SIGNAL(finished()), radio_op, SLOT(deleteLater()));
    QObject::connect(t1, SIGNAL(finished()), t1, SLOT(deleteLater()));
    QObject::connect(radio_op, SIGNAL(finished()), &a, SLOT(quit()));
    t1->start();

    QThread *t2 = new QThread;
    t2->setObjectName("audiowriter");
    audiowriter->moveToThread(t2);
    QObject::connect(t2, SIGNAL(started()), audiowriter, SLOT(run()));
    QObject::connect(audiowriter, SIGNAL(finished()), t2, SLOT(quit()));
    QObject::connect(audiowriter, SIGNAL(finished()), audiowriter, SLOT(deleteLater()));
    QObject::connect(t2, SIGNAL(finished()), t2, SLOT(deleteLater()));
    t2->start();

    QThread *t3 = new QThread;
    t3->setObjectName("audioreader");
    audioreader->moveToThread(t3);
    QObject::connect(t3, SIGNAL(started()), audioreader, SLOT(run()));
    QObject::connect(audioreader, SIGNAL(finished()), t3, SLOT(quit()));
    QObject::connect(audioreader, SIGNAL(finished()), audioreader, SLOT(deleteLater()));
    QObject::connect(t3, SIGNAL(finished()), t3, SLOT(deleteLater()));
    t3->start();

    MainWindow *w;
    if(!headless)
    {
        /// Init GUI
        ///
        w = new MainWindow(settings, logger, radio_channels);
        connectGuiSignals(telnet_server, audiowriter, audioreader, w,
                          mumbleclient, radio_op, logger);
        /// requires the slots to be set up
        w->initSettings();
        w->show();
        /* kiosk mode
        if(arguments.length() > 1 && (arguments.indexOf("-f") != -1))
        {
            w->showMaximized();
            w->setWindowFlags( Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
        }
        */
        w->activateWindow();
        w->raise();
    }

    /// Connect non-GUI signals
    ///
    connectCommandSignals(telnet_server, mumbleclient, radio_op);

    /// Signals independent of GUI or remote interface
    connectIndependentSignals(audiowriter, audioreader, radio_op, mumbleclient, telnet_server);


    /// Start remote command listener
    if(headless)
        telnet_server->start();

    int ret = a.exec();

    /// Cleanup on exit
    ///
    if(!headless)
        delete w;
    delete telnet_server;
    delete mumbleclient;
    radio_channels->saveConfig();
    delete radio_channels;
    settings->saveConfig();
    delete settings;
    logger->log(Logger::LogLevelInfo, "Stopping qradiolink");
    delete logger;
    return ret;
}

void connectIndependentSignals(AudioWriter *audiowriter, AudioReader *audioreader,
                               RadioController *radio_op, MumbleClient *mumbleclient, TelnetServer *telnet_server)
{
    QObject::connect(radio_op,SIGNAL(terminateConnections()),audiowriter,SLOT(stop()));
    QObject::connect(radio_op,SIGNAL(terminateConnections()),audioreader,SLOT(stop()));
    QObject::connect(radio_op,SIGNAL(terminateConnections()),telnet_server,SLOT(stop()));
    QObject::connect(radio_op, SIGNAL(writePCM(short*,int,bool,int)),
                     audiowriter, SLOT(writePCM(short*,int,bool, int)));
    QObject::connect(radio_op, SIGNAL(recordAudio(bool)),
                     audiowriter, SLOT(recordAudio(bool)));
    QObject::connect(radio_op, SIGNAL(setAudioReadMode(bool, bool,int, int)),
                     audioreader, SLOT(setReadMode(bool,bool,int, int)));
    QObject::connect(audioreader, SIGNAL(audioPCM(short*,int,int, bool)),
                     radio_op, SLOT(txAudio(short*,int,int, bool)));
    QObject::connect(radio_op, SIGNAL(voipDataOpus(unsigned char*,int)),
                     mumbleclient, SLOT(processOpusAudio(unsigned char*, int)));
    QObject::connect(radio_op, SIGNAL(voipVideoData(unsigned char*,int)),
                     mumbleclient, SLOT(processVideoFrame(unsigned char*, int)));
    QObject::connect(radio_op, SIGNAL(voipDataPCM(short*,int)),
                     mumbleclient, SLOT(processPCMAudio(short*,int)));
    QObject::connect(mumbleclient, SIGNAL(pcmAudio(short*,int,quint64)),
                     radio_op, SLOT(processVoipAudioFrame(short*, int, quint64)));
    QObject::connect(mumbleclient, SIGNAL(videoFrame(unsigned char*,int,quint64)),
                     radio_op, SLOT(processVoipVideoFrame(unsigned char*,int,quint64)));
    QObject::connect(mumbleclient,SIGNAL(newChannels(ChannelList)),
                     radio_op,SLOT(setChannels(ChannelList)));
    QObject::connect(mumbleclient,SIGNAL(onlineStations(StationList)),
                     radio_op,SLOT(setStations(StationList)));
    QObject::connect(radio_op, SIGNAL(newMumbleMessage(QString)),
                     mumbleclient, SLOT(newMumbleMessage(QString)));
    QObject::connect(mumbleclient, SIGNAL(textMessage(QString,bool)),
                     radio_op, SLOT(textMumble(QString,bool)));
}


void connectCommandSignals(TelnetServer *telnet_server, MumbleClient *mumbleclient,
                       RadioController *radio_op)
{
    QObject::connect(telnet_server->command_processor,SIGNAL(stopRadio()),radio_op,SLOT(stop()));
    QObject::connect(telnet_server->command_processor,SIGNAL(startTransmission()),
                     radio_op,SLOT(startTransmission()));
    QObject::connect(telnet_server->command_processor,SIGNAL(endTransmission()),
                     radio_op,SLOT(endTransmission()));
    QObject::connect(telnet_server->command_processor,SIGNAL(toggleRX(bool)),
                     radio_op,SLOT(toggleRX(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(toggleTX(bool)),
                     radio_op,SLOT(toggleTX(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(toggleTxModemMode(int)),
                     radio_op,SLOT(toggleTxMode(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(toggleRxModemMode(int)),
                     radio_op,SLOT(toggleRxMode(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(tuneFreq(qint64)),
                     radio_op,SLOT(tuneFreq(qint64)));
    QObject::connect(telnet_server->command_processor,SIGNAL(tuneTxFreq(qint64)),
                     radio_op,SLOT(tuneTxFreq(qint64)));
    QObject::connect(telnet_server->command_processor,SIGNAL(changeTxShift(qint64)),
                     radio_op,SLOT(changeTxShift(qint64)));
    QObject::connect(telnet_server->command_processor,SIGNAL(startAutoTuneFreq(int, int)),
                     radio_op,SLOT(startScan(int, int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(stopAutoTuneFreq()),
                     radio_op,SLOT(stopScan()));
    QObject::connect(telnet_server->command_processor,SIGNAL(fineTuneFreq(long)),radio_op,
                     SLOT(fineTuneFreq(long)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setTxPower(int, std::string)),radio_op,
                     SLOT(setTxPower(int, std::string)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setAgcAttack(int)),
                     radio_op,SLOT(setAgcAttack(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setAgcDecay(int)),
                     radio_op,SLOT(setAgcDecay(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setRxSensitivity(int)),
                     radio_op,SLOT(setRxSensitivity(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setSquelch(int)),
                     radio_op,SLOT(setSquelch(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setVolume(int)),
                     radio_op,SLOT(setVolume(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setTxVolume(int)),
                     radio_op,SLOT(setTxVolume(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setRxCTCSS(float)),
                     radio_op,SLOT(setRxCTCSS(float)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setTxCTCSS(float)),
                     radio_op,SLOT(setTxCTCSS(float)));
    QObject::connect(telnet_server->command_processor,SIGNAL(enableGUIConst(bool)),
                     radio_op,SLOT(enableGUIConst(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(enableGUIFFT(bool)),
                     radio_op,SLOT(enableGUIFFT(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(enableDuplex(bool)),
                     radio_op,SLOT(enableDuplex(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(enableRSSI(bool)),
                     radio_op,SLOT(enableRSSI(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(usePTTForVOIP(bool)),
                     radio_op,SLOT(usePTTForVOIP(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setVOIPForwarding(bool)),
                     radio_op,SLOT(setVOIPForwarding(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setVox(bool)),
                     radio_op,SLOT(setVox(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(toggleRepeat(bool)),
                     radio_op,SLOT(toggleRepeat(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setCarrierOffset(qint64)),
                     radio_op,SLOT(setCarrierOffset(qint64)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setTxCarrierOffset(qint64)),
                     radio_op,SLOT(setTxCarrierOffset(qint64)));
    QObject::connect(telnet_server->command_processor,SIGNAL(newFFTSize(int)),
                     radio_op,SLOT(setFFTSize(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setWaterfallFPS(int)),
                     radio_op,SLOT(setFFTPollTime(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setSampleRate(int)),
                     radio_op,SLOT(setRxSampleRate(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(newFilterWidth(int)),
                     radio_op,SLOT(setFilterWidth(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(enableAudioCompressor(bool)),
                     radio_op,SLOT(enableAudioCompressor(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(enableRelays(bool)),
                     radio_op,SLOT(enableRelays(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(calibrateRSSI(float)),
                     radio_op,SLOT(calibrateRSSI(float)));
    QObject::connect(telnet_server->command_processor,SIGNAL(connectToServer(QString, unsigned)),
                     mumbleclient,SLOT(connectToServer(QString, unsigned)));
    QObject::connect(telnet_server->command_processor,SIGNAL(disconnectFromServer()),
                     mumbleclient,SLOT(disconnectFromServer()));
    QObject::connect(telnet_server->command_processor,SIGNAL(setMute(bool)),
                     mumbleclient,SLOT(setMute(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(changeChannel(int)),
                     mumbleclient,SLOT(joinChannel(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(sendText(QString, bool)),
                     radio_op,SLOT(textData(QString, bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(newMumbleMessage(QString)),
                     mumbleclient,SLOT(newMumbleMessage(QString)));
    QObject::connect(telnet_server->command_processor,SIGNAL(newCommandMessage(QString,int)),
                     mumbleclient,SLOT(newCommandMessage(QString,int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(startMemoryTune(int)),
                     radio_op,SLOT(startMemoryScan(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(stopMemoryTune()),
                     radio_op,SLOT(stopMemoryScan()));
    QObject::connect(telnet_server->command_processor,SIGNAL(tuneMemoryChannel(radiochannel*)),
                     radio_op,SLOT(tuneMemoryChannel(radiochannel*)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setVoxLevel(int)),
                     radio_op,SLOT(setVoxLevel(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setVoipBitrate(int)),
                     radio_op,SLOT(setVoipBitrate(int)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setMuteForwardedAudio(bool)),
                     radio_op,SLOT(setMuteForwardedAudio(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setAudioRecord(bool)),
                     radio_op,SLOT(setAudioRecord(bool)));
    QObject::connect(telnet_server->command_processor,SIGNAL(setTxLimits(bool)),
                     radio_op,SLOT(setTxLimits(bool)));
    QObject::connect(mumbleclient,SIGNAL(commandMessage(QString,int)),
                     telnet_server->command_processor,SLOT(parseMumbleMessage(QString,int)));
}


void connectGuiSignals(TelnetServer *telnet_server, AudioWriter *audiowriter,
                       AudioReader *audioreader, MainWindow *w, MumbleClient *mumbleclient,
                       RadioController *radio_op, Logger *logger)
{
    /// GUI to radio and Mumble
    QObject::connect(w,SIGNAL(startTransmission()),radio_op,SLOT(startTransmission()));
    QObject::connect(w,SIGNAL(endTransmission()),radio_op,SLOT(endTransmission()));
    QObject::connect(w,SIGNAL(sendText(QString, bool)),radio_op,SLOT(textData(QString, bool)));
    QObject::connect(w,SIGNAL(toggleRX(bool)),radio_op,SLOT(toggleRX(bool)));
    QObject::connect(w,SIGNAL(toggleTX(bool)),radio_op,SLOT(toggleTX(bool)));
    QObject::connect(w,SIGNAL(toggleTxModemMode(int)),radio_op,SLOT(toggleTxMode(int)));
    QObject::connect(w,SIGNAL(toggleRxModemMode(int)),radio_op,SLOT(toggleRxMode(int)));
    QObject::connect(w,SIGNAL(tuneFreq(qint64)),radio_op,SLOT(tuneFreq(qint64)));
    QObject::connect(w,SIGNAL(tuneTxFreq(qint64)),radio_op,SLOT(tuneTxFreq(qint64)));
    QObject::connect(w,SIGNAL(changeTxShift(qint64)),radio_op,SLOT(changeTxShift(qint64)));
    QObject::connect(w,SIGNAL(startAutoTuneFreq(int, int)),radio_op,SLOT(startScan(int, int)));
    QObject::connect(w,SIGNAL(stopAutoTuneFreq()),radio_op,SLOT(stopScan()));
    QObject::connect(w,SIGNAL(startMemoryTune(int)), radio_op,SLOT(startMemoryScan(int)));
    QObject::connect(w,SIGNAL(stopMemoryTune()),radio_op,SLOT(stopMemoryScan()));
    QObject::connect(w,SIGNAL(fineTuneFreq(long)),radio_op,SLOT(fineTuneFreq(long)));
    QObject::connect(w,SIGNAL(setTxPower(int, std::string)),
                     radio_op,SLOT(setTxPower(int, std::string)));
    QObject::connect(w,SIGNAL(setBbGain(int)),radio_op,SLOT(setBbGain(int)));
    QObject::connect(w,SIGNAL(setIfGain(int)),radio_op,SLOT(setIfGain(int)));
    QObject::connect(w,SIGNAL(setAgcAttack(int)),radio_op,SLOT(setAgcAttack(int)));
    QObject::connect(w,SIGNAL(setAgcDecay(int)),radio_op,SLOT(setAgcDecay(int)));
    QObject::connect(w,SIGNAL(setRxSensitivity(int, std::string)),radio_op,SLOT(setRxSensitivity(int, std::string)));
    QObject::connect(w,SIGNAL(setSquelch(int)),radio_op,SLOT(setSquelch(int)));
    QObject::connect(w,SIGNAL(setVolume(int)),radio_op,SLOT(setVolume(int)));
    QObject::connect(w,SIGNAL(setTxVolume(int)),radio_op,SLOT(setTxVolume(int)));
    QObject::connect(w,SIGNAL(setVoipVolume(int)),radio_op,SLOT(setVoipVolume(int)));
    QObject::connect(w,SIGNAL(setRxCTCSS(float)),radio_op,SLOT(setRxCTCSS(float)));
    QObject::connect(w,SIGNAL(setTxCTCSS(float)),radio_op,SLOT(setTxCTCSS(float)));
    QObject::connect(w,SIGNAL(enableGUIConst(bool)),radio_op,SLOT(enableGUIConst(bool)));
    QObject::connect(w,SIGNAL(enableGUIFFT(bool)),radio_op,SLOT(enableGUIFFT(bool)));
    QObject::connect(w,SIGNAL(enableDuplex(bool)),radio_op,SLOT(enableDuplex(bool)));
    QObject::connect(w,SIGNAL(enableRSSI(bool)),radio_op,SLOT(enableRSSI(bool)));
    QObject::connect(w,SIGNAL(usePTTForVOIP(bool)),radio_op,SLOT(usePTTForVOIP(bool)));
    QObject::connect(w,SIGNAL(setVOIPForwarding(bool)),radio_op,SLOT(setVOIPForwarding(bool)));
    QObject::connect(w,SIGNAL(setVox(bool)),radio_op,SLOT(setVox(bool)));
    QObject::connect(w,SIGNAL(toggleRepeat(bool)),radio_op,SLOT(toggleRepeat(bool)));
    QObject::connect(w,SIGNAL(enableReverseShift(bool)),radio_op,SLOT(enableReverseShift(bool)));
    QObject::connect(w,SIGNAL(stopRadio()),radio_op,SLOT(stop()));
    QObject::connect(w,SIGNAL(setCarrierOffset(qint64)),radio_op,SLOT(setCarrierOffset(qint64)));
    QObject::connect(w,SIGNAL(newFFTSize(int)),radio_op,SLOT(setFFTSize(int)));
    QObject::connect(w,SIGNAL(setWaterfallFPS(int)),radio_op,SLOT(setFFTPollTime(int)));
    QObject::connect(w,SIGNAL(setSampleRate(int)),radio_op,SLOT(setRxSampleRate(int)));
    QObject::connect(w,SIGNAL(newFilterWidth(int)),radio_op,SLOT(setFilterWidth(int)));
    QObject::connect(w,SIGNAL(enableAudioCompressor(bool)),
                     radio_op,SLOT(enableAudioCompressor(bool)));
    QObject::connect(w,SIGNAL(enableRelays(bool)),radio_op,SLOT(enableRelays(bool)));
    QObject::connect(w,SIGNAL(calibrateRSSI(float)), radio_op,SLOT(calibrateRSSI(float)));
    QObject::connect(w,SIGNAL(setScanResumeTime(int)),
                     radio_op,SLOT(setScanResumeTime(int)));
    QObject::connect(w,SIGNAL(setAudioRecord(bool)),radio_op,SLOT(setAudioRecord(bool)));
    QObject::connect(w,SIGNAL(setVoipBitrate(int)),
                     radio_op,SLOT(setVoipBitrate(int)));
    QObject::connect(w,SIGNAL(setEndBeep(int)),
                     radio_op,SLOT(setEndBeep(int)));
    QObject::connect(w,SIGNAL(setMuteForwardedAudio(bool)),
                     radio_op,SLOT(setMuteForwardedAudio(bool)));
    QObject::connect(w,SIGNAL(setBlockBufferSize(int)),
                     radio_op,SLOT(setBlockBufferSize(int)));
    QObject::connect(w,SIGNAL(setRadioToT(int)),
                     radio_op,SLOT(setRadioToT(int)));
    QObject::connect(w,SIGNAL(setTotTxEnd(bool)),
                     radio_op,SLOT(setTotTxEnd(bool)));
    QObject::connect(w,SIGNAL(setTxLimits(bool)),
                     radio_op,SLOT(setTxLimits(bool)));
    QObject::connect(w,SIGNAL(pageUser(QString,QString)),
                     radio_op,SLOT(pageUser(QString, QString)));
    QObject::connect(w,SIGNAL(connectToServer(QString, unsigned)),
                     mumbleclient,SLOT(connectToServer(QString, unsigned)));
    QObject::connect(w,SIGNAL(disconnectFromServer()),mumbleclient,SLOT(disconnectFromServer()));
    QObject::connect(w,SIGNAL(disableRemote()),telnet_server,SLOT(stop()));
    QObject::connect(w,SIGNAL(enableRemote()),telnet_server,SLOT(start()));
    QObject::connect(w,SIGNAL(setMute(bool)),mumbleclient,SLOT(setMute(bool)));
    QObject::connect(w,SIGNAL(changeChannel(int)),mumbleclient,SLOT(joinChannel(int)));
    QObject::connect(w,SIGNAL(newMumbleMessage(QString)),
                     mumbleclient,SLOT(newMumbleMessage(QString)));
    QObject::connect(w,SIGNAL(setSelfDeaf(bool, bool)),
                     mumbleclient,SLOT(setSelfDeaf(bool, bool)));
    QObject::connect(w,SIGNAL(setSelfMute(bool)),
                     mumbleclient,SLOT(setSelfMute(bool)));
    QObject::connect(w,SIGNAL(restartAudioOutputThread()),
                     audiowriter,SLOT(restart()));
    QObject::connect(w,SIGNAL(restartAudioInputThread()),
                     audioreader,SLOT(restart()));

    /// Radio to GUI
    QObject::connect(radio_op, SIGNAL(printText(QString,bool)), w, SLOT(displayText(QString,bool)));
    QObject::connect(radio_op, SIGNAL(printCallsign(QString)), w, SLOT(displayCallsign(QString)));
    QObject::connect(radio_op, SIGNAL(videoImage(QImage)), w, SLOT(displayImage(QImage)));
    QObject::connect(radio_op, SIGNAL(displayReceiveStatus(bool)),
                     w, SLOT(displayReceiveStatus(bool)));
    QObject::connect(radio_op, SIGNAL(displayTransmitStatus(bool)),
                     w, SLOT(displayTransmitStatus(bool)));
    QObject::connect(radio_op, SIGNAL(displayDataReceiveStatus(bool)),
                     w, SLOT(displayDataReceiveStatus(bool)));
    QObject::connect(radio_op, SIGNAL(freqToGUI(long, long)),
                     w, SLOT(updateFreqGUI(long, long)));
    QObject::connect(radio_op, SIGNAL(newFFTData(float*,int)),
                     w, SLOT(newFFTData(float*,int)));
    QObject::connect(radio_op, SIGNAL(newRSSIValue(float)), w, SLOT(updateRSSI(float)));
    QObject::connect(radio_op, SIGNAL(newConstellationData(complex_vector*)),
                     w, SLOT(updateConstellation(complex_vector*)));
    QObject::connect(radio_op, SIGNAL(initError(QString)), w, SLOT(initError(QString)));
    QObject::connect(radio_op, SIGNAL(rxGainStages(gain_vector)),
                     w, SLOT(setRxGainStages(gain_vector)));
    QObject::connect(radio_op, SIGNAL(txGainStages(gain_vector)),
                     w, SLOT(setTxGainStages(gain_vector)));
    QObject::connect(radio_op, SIGNAL(tuneToMemoryChannel(radiochannel*)),
                     w, SLOT(tuneToMemoryChannel(radiochannel*)));
    QObject::connect(radio_op, SIGNAL(newPageMessage(QString,QString)),
                     w, SLOT(displayPageMessage(QString,QString)));

    /// Mumble to GUI
    QObject::connect(mumbleclient,SIGNAL(onlineStations(StationList)),
                     w,SLOT(updateOnlineStations(StationList)));
    QObject::connect(mumbleclient,SIGNAL(userSpeaking(quint64)),w,SLOT(userSpeaking(quint64)));
    QObject::connect(mumbleclient,SIGNAL(textMessage(QString,bool)),
                     w,SLOT(displayVOIPText(QString,bool)));
    QObject::connect(mumbleclient,SIGNAL(connectedToServer(QString)),
                     w,SLOT(connectedToServer(QString)));
    QObject::connect(mumbleclient,SIGNAL(newChannels(ChannelList)),
                     w,SLOT(updateChannels(ChannelList)));
    QObject::connect(mumbleclient,SIGNAL(joinedChannel(quint64)),w,SLOT(joinedChannel(quint64)));
    QObject::connect(mumbleclient,SIGNAL(disconnected()),w,SLOT(disconnectedFromServer()));
    QObject::connect(logger,SIGNAL(applicationLog(QString)),w,SLOT(applicationLog(QString)));
    QObject::connect(telnet_server->command_processor,SIGNAL(tuneFreq(qint64)),
                     w,SLOT(updateGUIFreq(qint64)));
}
