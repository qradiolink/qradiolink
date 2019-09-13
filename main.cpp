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

#include "mainwindow.h"
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
#include "dtmfdecoder.h"
#include "databaseapi.h"
#include "serverwrapper.h"
#include "config_defines.h"
#if 0
#include "controller.h"
#endif
#include "mumbleclient.h"
#include "audioop.h"
#include "dtmfcommand.h"
#include "station.h"
#include "channel.h"
#include "radioop.h"


#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void logMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
#else
void logMessage(QtMsgType type, const char *msg)
#endif
{
    QString txt;
    switch (type) {
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

    typedef QVector<Station> StationList;
    qRegisterMetaType<StationList>("StationList");
    typedef std::vector<std::complex<float>> complex_vector;
    qRegisterMetaType<complex_vector>("complex_vector");


    QApplication a(argc, argv);
    a.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 0px solid white; }");
    QStringList arguments = QCoreApplication::arguments();
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-Normal.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-Bold.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-BoldItalic.otf");
    QFontDatabase::addApplicationFont(":/fonts/res/LiquidCrystal-NormalItalic.otf");


    std::string start_time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss").toStdString();
    std::cout << "Starting qradiolink instance: " << start_time << std::endl;
    DatabaseApi db;
    Settings *settings = new Settings;
    settings->readConfig();
    MumbleClient *client = new MumbleClient(settings);
    RadioOp *radio_op = new RadioOp(settings);
    MainWindow *w = new MainWindow(settings);
    w->setWindowTitle("QRadioLink");

    w->show();
    if(arguments.length() > 1 && arguments.at(1) == "-f")
    {
        w->showMaximized();
        w->setWindowFlags( Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    }
    w->activateWindow();
    w->raise();

    /* Uncomment later
    DtmfCommand *dtmfcommand = new DtmfCommand(settings, &db,client);
    QObject::connect(client,SIGNAL(channelReady(int)),dtmfcommand,SLOT(channelReady(int)));
    QObject::connect(client,SIGNAL(newStation(Station*)),dtmfcommand,SLOT(newStation(Station*)));
    QObject::connect(client,SIGNAL(leftStation(Station*)),dtmfcommand,SLOT(leftStation(Station*)));
    */
    /*
    QThread *t1= new QThread;
    DtmfDecoder *decoder = new DtmfDecoder(settings);
    if(settings->_use_dtmf)
    {

        decoder->moveToThread(t1);

        QObject::connect(decoder,SIGNAL(haveCall(QVector<char>*)),dtmfcommand,SLOT(haveCall(QVector<char>*)));
        QObject::connect(decoder,SIGNAL(haveCommand(QVector<char>*)),dtmfcommand,SLOT(haveCommand(QVector<char>*)));
        QObject::connect(dtmfcommand,SIGNAL(readyInput()),decoder,SLOT(resetInput()));

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



    QThread *t3 = new QThread;
    AudioOp *audio_op = new AudioOp(settings);
    audio_op->moveToThread(t3);
    QObject::connect(audio_op,SIGNAL(audioData(short*,short)),client,SLOT(processAudio(short*,short)));
    QObject::connect(client,SIGNAL(pcmAudio(short*,short)),audio_op,SLOT(pcmAudio(short*,short)));
    QObject::connect(t3, SIGNAL(started()), audio_op, SLOT(run()));
    QObject::connect(audio_op, SIGNAL(finished()), t3, SLOT(quit()));
    QObject::connect(audio_op, SIGNAL(finished()), audio_op, SLOT(deleteLater()));
    QObject::connect(t3, SIGNAL(finished()), t3, SLOT(deleteLater()));
    t3->start();
    */



    QThread *t4 = new QThread;
    t4->setObjectName("radioop");

    radio_op->moveToThread(t4);
    QObject::connect(t4, SIGNAL(started()), radio_op, SLOT(run()));
    QObject::connect(radio_op, SIGNAL(finished()), t4, SLOT(quit()));
    QObject::connect(radio_op, SIGNAL(finished()), radio_op, SLOT(deleteLater()));
    QObject::connect(t4, SIGNAL(finished()), t4, SLOT(deleteLater()));
    t4->start();

    QObject::connect(w,SIGNAL(startTransmission()),radio_op,SLOT(startTransmission()));
    QObject::connect(w,SIGNAL(endTransmission()),radio_op,SLOT(endTransmission()));
    QObject::connect(w,SIGNAL(sendText(QString, bool)),radio_op,SLOT(textData(QString, bool)));
    QObject::connect(w,SIGNAL(toggleRX(bool)),radio_op,SLOT(toggleRX(bool)));
    QObject::connect(w,SIGNAL(toggleTX(bool)),radio_op,SLOT(toggleTX(bool)));
    QObject::connect(w,SIGNAL(toggleTxModemMode(int)),radio_op,SLOT(toggleTxMode(int)));
    QObject::connect(w,SIGNAL(toggleRxModemMode(int)),radio_op,SLOT(toggleRxMode(int)));
    QObject::connect(w,SIGNAL(tuneFreq(qint64)),radio_op,SLOT(tuneFreq(qint64)));
    QObject::connect(w,SIGNAL(tuneTxFreq(qint64)),radio_op,SLOT(tuneTxFreq(qint64)));
    QObject::connect(w,SIGNAL(startAutoTuneFreq(int)),radio_op,SLOT(startAutoTune(int)));
    QObject::connect(w,SIGNAL(stopAutoTuneFreq()),radio_op,SLOT(stopAutoTune()));
    QObject::connect(w,SIGNAL(fineTuneFreq(long)),radio_op,SLOT(fineTuneFreq(long)));
    QObject::connect(w,SIGNAL(setTxPower(int)),radio_op,SLOT(setTxPower(int)));
    //QObject::connect(w,SIGNAL(setBbGain(int)),radio_op,SLOT(setBbGain(int)));
    QObject::connect(w,SIGNAL(setRxSensitivity(int)),radio_op,SLOT(setRxSensitivity(int)));
    QObject::connect(w,SIGNAL(setSquelch(int)),radio_op,SLOT(setSquelch(int)));
    QObject::connect(w,SIGNAL(setVolume(int)),radio_op,SLOT(setVolume(int)));
    QObject::connect(w,SIGNAL(setRxCTCSS(float)),radio_op,SLOT(setRxCTCSS(float)));
    QObject::connect(w,SIGNAL(setTxCTCSS(float)),radio_op,SLOT(setTxCTCSS(float)));
    QObject::connect(w,SIGNAL(enableGUIConst(bool)),radio_op,SLOT(enableGUIConst(bool)));
    QObject::connect(w,SIGNAL(enableGUIFFT(bool)),radio_op,SLOT(enableGUIFFT(bool)));
    QObject::connect(w,SIGNAL(usePTTForVOIP(bool)),radio_op,SLOT(usePTTForVOIP(bool)));
    QObject::connect(w,SIGNAL(setVOIPForwarding(bool)),radio_op,SLOT(setVOIPForwarding(bool)));
    QObject::connect(w,SIGNAL(setVox(bool)),radio_op,SLOT(setVox(bool)));
    QObject::connect(w,SIGNAL(toggleRepeat(bool)),radio_op,SLOT(toggleRepeat(bool)));
    QObject::connect(w,SIGNAL(stopRadio()),radio_op,SLOT(stop()));
    QObject::connect(w,SIGNAL(setCarrierOffset(qint64)),radio_op,SLOT(setCarrierOffset(qint64)));
    QObject::connect(w,SIGNAL(newFFTSize(int)),radio_op,SLOT(setFFTSize(int)));
    QObject::connect(w,SIGNAL(setWaterfallFPS(int)),radio_op,SLOT(setFFTPollTime(int)));
    QObject::connect(w,SIGNAL(setSampleRate(int)),radio_op,SLOT(setRxSampleRate(int)));
    QObject::connect(radio_op, SIGNAL(printText(QString,bool)), w, SLOT(displayText(QString,bool)));
    QObject::connect(radio_op, SIGNAL(printCallsign(QString)), w, SLOT(displayCallsign(QString)));
    QObject::connect(radio_op, SIGNAL(videoImage(QImage)), w, SLOT(displayImage(QImage)));
    QObject::connect(radio_op, SIGNAL(displayReceiveStatus(bool)), w, SLOT(displayReceiveStatus(bool)));
    QObject::connect(radio_op, SIGNAL(displayTransmitStatus(bool)), w, SLOT(displayTransmitStatus(bool)));
    QObject::connect(radio_op, SIGNAL(displayDataReceiveStatus(bool)), w, SLOT(displayDataReceiveStatus(bool)));
    QObject::connect(radio_op, SIGNAL(freqToGUI(long long, long)), w, SLOT(updateFreqGUI(long long, long)));
    QObject::connect(radio_op, SIGNAL(pingServer()), client, SLOT(pingServer()));
    QObject::connect(client, SIGNAL(pcmAudio(short*,int,quint64)), radio_op, SLOT(processVoipAudioFrame(short*, int, quint64)));
    QObject::connect(radio_op, SIGNAL(voipData(short*,int)), client, SLOT(processAudio(short*,int)));
    QObject::connect(radio_op, SIGNAL(newFFTData(std::complex<float>*,int)), w, SLOT(newFFTData(std::complex<float>*,int)));
    QObject::connect(radio_op, SIGNAL(newRSSIValue(float)), w, SLOT(updateRSSI(float)));
    QObject::connect(radio_op, SIGNAL(newConstellationData(complex_vector*)), w, SLOT(updateConstellation(complex_vector*)));
    QObject::connect(radio_op, SIGNAL(initError(QString)), w, SLOT(initError(QString)));

    QObject::connect(client,SIGNAL(onlineStations(StationList)),w,SLOT(updateOnlineStations(StationList)));
    QObject::connect(client,SIGNAL(onlineStations(StationList)),radio_op,SLOT(setStations(StationList)));
    QObject::connect(client,SIGNAL(textMessage(QString,bool)),w,SLOT(displayText(QString,bool)));
    QObject::connect(client,SIGNAL(newChannel(Channel*)),w,SLOT(newChannel(Channel*)));
    QObject::connect(client,SIGNAL(newChannel(Channel*)),radio_op,SLOT(addChannel(Channel*)));

    QObject::connect(w,SIGNAL(connectToServer(QString, unsigned)),client,SLOT(connectToServer(QString, unsigned)));
    QObject::connect(w,SIGNAL(disconnectFromServer()),client,SLOT(disconnectFromServer()));
    QObject::connect(w,SIGNAL(setMute(bool)),client,SLOT(setMute(bool)));
    QObject::connect(w,SIGNAL(changeChannel(int)),client,SLOT(joinChannel(int)));


    int ret = a.exec();

    client->disconnectFromServer();
    delete w;
    delete client;
    return ret;
}
