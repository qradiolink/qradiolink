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
#include "databaseapi.h"
#include "config_defines.h"
#include "mumbleclient.h"
#include "audiowriter.h"
#include "audioreader.h"
#include "station.h"
#include "mumblechannel.h"
#include "radiochannel.h"
#include "radiocontroller.h"


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
    typedef QVector<MumbleChannel*> ChannelList;
    qRegisterMetaType<StationList>("StationList");
    qRegisterMetaType<ChannelList>("ChannelList");
    typedef std::vector<std::complex<float>> complex_vector;
    qRegisterMetaType<complex_vector>("complex_vector");


    QApplication a(argc, argv);

    QStringList arguments = QCoreApplication::arguments();
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-Normal.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-Bold.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-BoldItalic.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-NormalItalic.otf");


    std::string start_time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss").toStdString();
    std::cout << "Starting qradiolink instance: " << start_time << std::endl;
    //DatabaseApi db;
    Settings *settings = new Settings;
    RadioChannels *radio_channels = new RadioChannels;
    settings->readConfig();
    MumbleClient *mumbleclient = new MumbleClient(settings);
    RadioController *radio_op = new RadioController(settings);
    AudioWriter *audiowriter = new AudioWriter;
    AudioReader *audioreader = new AudioReader;
    MainWindow *w = new MainWindow(settings, radio_channels);

    w->show();
    if(arguments.length() > 1 && arguments.at(1) == "-f")
    {
        w->showMaximized();
        w->setWindowFlags( Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    }
    w->activateWindow();
    w->raise();

    /*
    QThread *t1= new QThread;
    DtmfDecoder *decoder = new DtmfDecoder(settings);
    if(settings->_use_dtmf)
    {

        decoder->moveToThread(t1);
        QObject::connect(t1, SIGNAL(started()), decoder, SLOT(run()));
        QObject::connect(decoder, SIGNAL(finished()), t1, SLOT(quit()));
        QObject::connect(decoder, SIGNAL(finished()), decoder, SLOT(deleteLater()));
        QObject::connect(t1, SIGNAL(finished()), t1, SLOT(deleteLater()));
        t1->start();
    }


    QThread *t2= new QThread;
    ServerWrapper *telnet_server_wrapper = new ServerWrapper(settings, &db);
    telnet_server_wrapper->moveToThread(t2);
    QObject::connect(dtmfcommand,SIGNAL(speak(QString)),telnet_server_wrapper,SLOT(addSpeech(QString)));
    QObject::connect(telnet_server_wrapper,SIGNAL(pingServer()),client,SLOT(pingServer()));
    QObject::connect(client,SIGNAL(onlineStations(StationList)),telnet_server_wrapper,SLOT(updateOnlineStations(StationList)));
    QObject::connect(dtmfcommand,SIGNAL(tellStations()),telnet_server_wrapper,SLOT(tellOnlineStations()));
    //QObject::connect(telnet_server_wrapper,SIGNAL(joinConference(int,int,int)),controller,SLOT(joinConference(int,int,int)));
    //QObject::connect(telnet_server_wrapper,SIGNAL(leaveConference(int,int,int)),controller,SLOT(leaveConference(int,int,int)));
    QObject::connect(t2, SIGNAL(started()), telnet_server_wrapper, SLOT(run()));
    QObject::connect(telnet_server_wrapper, SIGNAL(finished()), t2, SLOT(quit()));
    QObject::connect(telnet_server_wrapper, SIGNAL(finished()), telnet_server_wrapper, SLOT(deleteLater()));
    QObject::connect(t2, SIGNAL(finished()), t2, SLOT(deleteLater()));
    t2->start();

    */



    QThread *t4 = new QThread;
    t4->setObjectName("radioop");
    radio_op->moveToThread(t4);
    QObject::connect(t4, SIGNAL(started()), radio_op, SLOT(run()));
    QObject::connect(radio_op, SIGNAL(finished()), t4, SLOT(quit()));
    QObject::connect(radio_op, SIGNAL(finished()), radio_op, SLOT(deleteLater()));
    QObject::connect(t4, SIGNAL(finished()), t4, SLOT(deleteLater()));
    t4->start();

    QThread *t5 = new QThread;
    t5->setObjectName("audiowriter");
    audiowriter->moveToThread(t5);
    QObject::connect(t5, SIGNAL(started()), audiowriter, SLOT(run()));
    QObject::connect(audiowriter, SIGNAL(finished()), t5, SLOT(quit()));
    QObject::connect(audiowriter, SIGNAL(finished()), audiowriter, SLOT(deleteLater()));
    QObject::connect(t5, SIGNAL(finished()), t5, SLOT(deleteLater()));
    t5->start();

    QThread *t6 = new QThread;
    t6->setObjectName("audioreader");
    audioreader->moveToThread(t6);
    QObject::connect(t6, SIGNAL(started()), audioreader, SLOT(run()));
    QObject::connect(audioreader, SIGNAL(finished()), t6, SLOT(quit()));
    QObject::connect(audioreader, SIGNAL(finished()), audioreader, SLOT(deleteLater()));
    QObject::connect(t6, SIGNAL(finished()), t6, SLOT(deleteLater()));
    t6->start();

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
    QObject::connect(mumbleclient, SIGNAL(pcmAudio(short*,int,quint64)), radio_op, SLOT(processVoipAudioFrame(short*, int, quint64)));
    QObject::connect(radio_op, SIGNAL(voipDataPCM(short*,int)), mumbleclient, SLOT(processPCMAudio(short*,int)));
    QObject::connect(radio_op, SIGNAL(writePCM(short*,int,bool,int)), audiowriter, SLOT(writePCM(short*,int,bool, int)));
    QObject::connect(radio_op, SIGNAL(voipDataOpus(unsigned char*,int)), mumbleclient, SLOT(processOpusAudio(unsigned char*, int)));
    QObject::connect(radio_op, SIGNAL(newFFTData(float*,int)), w, SLOT(newFFTData(float*,int)));
    QObject::connect(radio_op, SIGNAL(newRSSIValue(float)), w, SLOT(updateRSSI(float)));
    QObject::connect(radio_op, SIGNAL(newConstellationData(complex_vector*)), w, SLOT(updateConstellation(complex_vector*)));
    QObject::connect(radio_op, SIGNAL(initError(QString)), w, SLOT(initError(QString)));

    QObject::connect(mumbleclient,SIGNAL(onlineStations(StationList)),w,SLOT(updateOnlineStations(StationList)));
    QObject::connect(mumbleclient,SIGNAL(userSpeaking(quint64)),w,SLOT(userSpeaking(quint64)));
    QObject::connect(mumbleclient,SIGNAL(onlineStations(StationList)),radio_op,SLOT(setStations(StationList)));
    QObject::connect(mumbleclient,SIGNAL(textMessage(QString,bool)),w,SLOT(displayVOIPText(QString,bool)));
    QObject::connect(mumbleclient,SIGNAL(connectedToServer(QString)),w,SLOT(connectedToServer(QString)));
    QObject::connect(mumbleclient,SIGNAL(newChannels(ChannelList)),w,SLOT(updateChannels(ChannelList)));
    QObject::connect(mumbleclient,SIGNAL(joinedChannel(quint64)),w,SLOT(joinedChannel(quint64)));
    QObject::connect(mumbleclient,SIGNAL(newChannels(ChannelList)),radio_op,SLOT(setChannels(ChannelList)));
    QObject::connect(mumbleclient,SIGNAL(disconnected()),w,SLOT(disconnectedFromServer()));

    QObject::connect(w,SIGNAL(connectToServer(QString, unsigned)),mumbleclient,SLOT(connectToServer(QString, unsigned)));
    QObject::connect(w,SIGNAL(disconnectFromServer()),mumbleclient,SLOT(disconnectFromServer()));
    QObject::connect(w,SIGNAL(setMute(bool)),mumbleclient,SLOT(setMute(bool)));
    QObject::connect(w,SIGNAL(changeChannel(int)),mumbleclient,SLOT(joinChannel(int)));

    w->initSettings();
    int ret = a.exec();

    mumbleclient->disconnectFromServer();
    audiowriter->stop();
    audioreader->stop();
    delete w;
    delete mumbleclient;
    delete radio_channels;
    delete settings;
    return ret;
}
