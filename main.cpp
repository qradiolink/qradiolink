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
#include <QDateTime>
#include <QString>
#include <QVector>
#include <QMetaType>
#include <QFile>
#include <QtGlobal>
#include <QTextStream>
#include <iostream>
#include "mainwindow.h"
#include "dtmfdecoder.h"
#include "mumbleclient.h"
#include "audiowriter.h"
#include "audioreader.h"
#include "station.h"
#include "mumblechannel.h"
#include "radiochannel.h"
#include "radiocontroller.h"
#include "telnetserver.h"


#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void logMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
#else
void logMessage(QtMsgType type, const char *msg)
{
#endif

    QString txt;
    switch (type) {
    case QtInfoMsg:
        txt = QString("Info: %1").arg(msg);
        break;
    case QtDebugMsg:
        txt = QString("Debug: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1").arg(msg);
    break;
    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(msg);
    break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
    break;
    }
    QFile outFile("qradiolink.log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
}

int main(int argc, char *argv[])
{
#if 0
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    qInstallMessageHandler(logMessage);
#else
    qInstallMsgHandler(logMessage);
#endif
#endif

    typedef QVector<Station*> StationList;
    qRegisterMetaType<StationList>("StationList");
    typedef QVector<MumbleChannel*> ChannelList;
    qRegisterMetaType<ChannelList>("ChannelList");
    typedef std::vector<std::complex<float>> complex_vector;
    qRegisterMetaType<complex_vector>("complex_vector");
    typedef std::vector<std::string> string_vector;
    qRegisterMetaType<string_vector>("string_vector");


    QApplication a(argc, argv);

    QStringList arguments = QCoreApplication::arguments();
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-Normal.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-Bold.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-BoldItalic.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-NormalItalic.otf");


    std::string start_time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss").toStdString();
    std::cout << "Starting qradiolink instance: " << start_time << std::endl;

    Settings *settings = new Settings;
    settings->readConfig();
    RadioChannels *radio_channels = new RadioChannels;
    radio_channels->readConfig();
    MumbleClient *mumbleclient = new MumbleClient(settings);
    RadioController *radio_op = new RadioController(settings);
    AudioWriter *audiowriter = new AudioWriter(settings);
    AudioReader *audioreader = new AudioReader(settings);
    TelnetServer *telnet_server = new TelnetServer(settings);
    telnet_server->start(); // FIXME
    MainWindow *w = new MainWindow(settings, radio_channels);

    w->show();
    if(arguments.length() > 1 && arguments.at(1) == "-f")
    {
        w->showMaximized();
        w->setWindowFlags( Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    }
    w->activateWindow();
    w->raise();



    QThread *t1 = new QThread;
    t1->setObjectName("radioop");
    radio_op->moveToThread(t1);
    QObject::connect(t1, SIGNAL(started()), radio_op, SLOT(run()));
    QObject::connect(radio_op, SIGNAL(finished()), t1, SLOT(quit()));
    QObject::connect(radio_op, SIGNAL(finished()), radio_op, SLOT(deleteLater()));
    QObject::connect(t1, SIGNAL(finished()), t1, SLOT(deleteLater()));
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
    QObject::connect(w,SIGNAL(startMemoryTune(RadioChannels*, int)),radio_op,SLOT(startMemoryScan(RadioChannels*, int)));
    QObject::connect(w,SIGNAL(stopMemoryTune()),radio_op,SLOT(stopMemoryScan()));
    QObject::connect(w,SIGNAL(fineTuneFreq(long)),radio_op,SLOT(fineTuneFreq(long)));
    QObject::connect(w,SIGNAL(setTxPower(int)),radio_op,SLOT(setTxPower(int)));
    //QObject::connect(w,SIGNAL(setBbGain(int)),radio_op,SLOT(setBbGain(int)));
    QObject::connect(w,SIGNAL(setRxSensitivity(int)),radio_op,SLOT(setRxSensitivity(int)));
    QObject::connect(w,SIGNAL(setSquelch(int)),radio_op,SLOT(setSquelch(int)));
    QObject::connect(w,SIGNAL(setVolume(int)),radio_op,SLOT(setVolume(int)));
    QObject::connect(w,SIGNAL(setTxVolume(int)),radio_op,SLOT(setTxVolume(int)));
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
    QObject::connect(w,SIGNAL(stopRadio()),radio_op,SLOT(stop()));
    QObject::connect(w,SIGNAL(setCarrierOffset(qint64)),radio_op,SLOT(setCarrierOffset(qint64)));
    QObject::connect(w,SIGNAL(newFFTSize(int)),radio_op,SLOT(setFFTSize(int)));
    QObject::connect(w,SIGNAL(setWaterfallFPS(int)),radio_op,SLOT(setFFTPollTime(int)));
    QObject::connect(w,SIGNAL(setSampleRate(int)),radio_op,SLOT(setRxSampleRate(int)));
    QObject::connect(w,SIGNAL(newFilterWidth(int)),radio_op,SLOT(setFilterWidth(int)));
    QObject::connect(w,SIGNAL(enableAudioCompressor(bool)),radio_op,SLOT(enableAudioCompressor(bool)));
    QObject::connect(w,SIGNAL(enableRelays(bool)),radio_op,SLOT(enableRelays(bool)));
    QObject::connect(w,SIGNAL(calibrateRSSI(float)), radio_op,SLOT(calibrateRSSI(float)));
    QObject::connect(w,SIGNAL(connectToServer(QString, unsigned)),mumbleclient,SLOT(connectToServer(QString, unsigned)));
    QObject::connect(w,SIGNAL(disconnectFromServer()),mumbleclient,SLOT(disconnectFromServer()));
    QObject::connect(w,SIGNAL(terminateConnections()),audiowriter,SLOT(stop()));
    QObject::connect(w,SIGNAL(terminateConnections()),audioreader,SLOT(stop()));
    QObject::connect(w,SIGNAL(terminateConnections()),telnet_server,SLOT(stop()));
    QObject::connect(w,SIGNAL(setMute(bool)),mumbleclient,SLOT(setMute(bool)));
    QObject::connect(w,SIGNAL(changeChannel(int)),mumbleclient,SLOT(joinChannel(int)));

    QObject::connect(radio_op, SIGNAL(setAudioReadMode(bool,bool,int)), audioreader, SLOT(setReadMode(bool,bool,int)));
    QObject::connect(audioreader, SIGNAL(audioPCM(short*,int,int, bool)), radio_op, SLOT(txAudio(short*,int,int, bool)));
    QObject::connect(radio_op, SIGNAL(printText(QString,bool)), w, SLOT(displayText(QString,bool)));
    QObject::connect(radio_op, SIGNAL(printCallsign(QString)), w, SLOT(displayCallsign(QString)));
    QObject::connect(radio_op, SIGNAL(videoImage(QImage)), w, SLOT(displayImage(QImage)));
    QObject::connect(radio_op, SIGNAL(displayReceiveStatus(bool)), w, SLOT(displayReceiveStatus(bool)));
    QObject::connect(radio_op, SIGNAL(displayTransmitStatus(bool)), w, SLOT(displayTransmitStatus(bool)));
    QObject::connect(radio_op, SIGNAL(displayDataReceiveStatus(bool)), w, SLOT(displayDataReceiveStatus(bool)));
    QObject::connect(radio_op, SIGNAL(freqToGUI(long long, long)), w, SLOT(updateFreqGUI(long long, long)));
    QObject::connect(radio_op, SIGNAL(pingServer()), mumbleclient, SLOT(pingServer()));
    QObject::connect(radio_op, SIGNAL(voipDataPCM(short*,int)), mumbleclient, SLOT(processPCMAudio(short*,int)));
    QObject::connect(radio_op, SIGNAL(writePCM(short*,int,bool,int)), audiowriter, SLOT(writePCM(short*,int,bool, int)));
    QObject::connect(radio_op, SIGNAL(voipDataOpus(unsigned char*,int)), mumbleclient, SLOT(processOpusAudio(unsigned char*, int)));
    QObject::connect(radio_op, SIGNAL(newFFTData(float*,int)), w, SLOT(newFFTData(float*,int)));
    QObject::connect(radio_op, SIGNAL(newRSSIValue(float)), w, SLOT(updateRSSI(float)));
    QObject::connect(radio_op, SIGNAL(newConstellationData(complex_vector*)), w, SLOT(updateConstellation(complex_vector*)));
    QObject::connect(radio_op, SIGNAL(initError(QString)), w, SLOT(initError(QString)));
    QObject::connect(radio_op, SIGNAL(rxGainStages(string_vector)), w, SLOT(setRxGainStages(string_vector)));
    QObject::connect(radio_op, SIGNAL(txGainStages(string_vector)), w, SLOT(setTxGainStages(string_vector)));

    QObject::connect(mumbleclient,SIGNAL(onlineStations(StationList)),w,SLOT(updateOnlineStations(StationList)));
    QObject::connect(mumbleclient,SIGNAL(userSpeaking(quint64)),w,SLOT(userSpeaking(quint64)));
    QObject::connect(mumbleclient, SIGNAL(pcmAudio(short*,int,quint64)), radio_op, SLOT(processVoipAudioFrame(short*, int, quint64)));
    QObject::connect(mumbleclient,SIGNAL(onlineStations(StationList)),radio_op,SLOT(setStations(StationList)));
    QObject::connect(mumbleclient,SIGNAL(textMessage(QString,bool)),w,SLOT(displayVOIPText(QString,bool)));
    QObject::connect(mumbleclient,SIGNAL(connectedToServer(QString)),w,SLOT(connectedToServer(QString)));
    QObject::connect(mumbleclient,SIGNAL(newChannels(ChannelList)),w,SLOT(updateChannels(ChannelList)));
    QObject::connect(mumbleclient,SIGNAL(joinedChannel(quint64)),w,SLOT(joinedChannel(quint64)));
    QObject::connect(mumbleclient,SIGNAL(newChannels(ChannelList)),radio_op,SLOT(setChannels(ChannelList)));
    QObject::connect(mumbleclient,SIGNAL(disconnected()),w,SLOT(disconnectedFromServer()));


    w->initSettings();
    int ret = a.exec();


    delete w;
    delete telnet_server;
    delete mumbleclient;
    radio_channels->saveConfig();
    delete radio_channels;
    settings->saveConfig();
    delete settings;
    return ret;
}
