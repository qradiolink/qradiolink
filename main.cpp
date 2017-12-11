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
#include "radioop.h"
#include <gnuradio/qtgui/const_sink_c.h>
#include <gnuradio/qtgui/sink_c.h>
#include <gnuradio/qtgui/number_sink.h>

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


    QApplication a(argc, argv);

    QString start_time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss");
    qDebug() << start_time;
    DatabaseApi db;
    Settings *settings = new Settings;
    settings->readConfig();
    MumbleClient *client = new MumbleClient(settings);
    MainWindow *w = new MainWindow(settings);
    w->setWindowTitle("QRadioLink");

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

    const std::string const_name = "const";
    gr::qtgui::const_sink_c::sptr const_gui = gr::qtgui::const_sink_c::make(256, const_name,1, w->get_const_gui());
    const_gui->set_size(600,400);
    const_gui->set_update_time(0.2);
    const_gui->qwidget()->setStyleSheet(
                "QwtPlotCanvas { background-color: #E1DFFF;  \
                border: 1px solid White; \
                border-radius: 10px; \
                } \
                DisplayPlot { \
                qproperty-zoomer_color: black; \
                qproperty-line_color1: #01147F; \
                qproperty-line_color2: magenta; \
                qproperty-line_color3: #FF6905; \
                qproperty-line_style2: DashLine; \
                qproperty-line_style3: DotLine; \
                qproperty-line_width1: 1; \
                qproperty-line_width2: 1; \
                qproperty-line_width3: 1; \
                qproperty-marker_alpha1: 210; \
                qproperty-marker_alpha2: 0; \
                qproperty-marker_alpha3: 0; \
                qproperty-axes_label_font_size: 12; \
                } ");


    gr::qtgui::number_sink::sptr rssi_gui = gr::qtgui::number_sink::make(4,0.5,gr::qtgui::NUM_GRAPH_HORIZ,1,w->get_rssi_gui());
    rssi_gui->set_max(0,10);
    rssi_gui->set_min(0,-120);
    rssi_gui->set_label(0,"RSSI");
    w->get_rssi_gui()->resize(700,90);
    rssi_gui->qwidget()->resize(700,90);
    rssi_gui->set_update_time(0.3);
    rssi_gui->set_color(0, "#ff0000", "#0000cc");
    rssi_gui->qwidget()->setStyleSheet(
            "QwtThermo { background-color: #E1DFFF;" \
            "color: #FF6905;}");

    const std::string fft_name = "fft";
    gr::qtgui::sink_c::sptr fft_gui = gr::qtgui::sink_c::make(8096,gr::filter::firdes::WIN_BLACKMAN_HARRIS,
                                                          0,1000000,fft_name,true,true,true,false,w->get_fft_gui());

    fft_gui->set_update_time(0.1);
    fft_gui->set_fft_power_db(-140,0);
    QRect xy = w->geometry();
    w->get_fft_gui()->resize(xy.right() -xy.left(),xy.bottom()-xy.top()-100);
    fft_gui->qwidget()->resize(xy.right() -xy.left()-50,xy.bottom()-xy.top()-206);
    fft_gui->qwidget()->setStyleSheet(
                "QwtPlotCanvas { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #FFFEA2, stop: 1 #0385FF); \
                border: 1px solid White; \
                border-radius: 10px; } \
                DisplayPlot { \
                qproperty-zoomer_color: black; \
                qproperty-line_color1: #A4051F; \
                qproperty-line_color2: #FF710C; \
                qproperty-line_color3: #FF6905; \
                qproperty-line_style1: SolidLine; \
                qproperty-line_style2: DashLine; \
                qproperty-line_style3: DotLine; \
                qproperty-line_width1: 1; \
                qproperty-line_width2: 2; \
                qproperty-line_width3: 2; \
                qproperty-marker_alpha1: 100; \
                qproperty-marker_alpha2: 150; \
                qproperty-marker_alpha3: 150; \
                qproperty-axes_label_font_size: 12; \
                } \
                WaterfallDisplayPlot { \
                qproperty-intensity_color_map_type1: 5; \
                    qproperty-low_intensity_color: #0B0041; \
                qproperty-high_intensity_color: #FFFB00; \
                } \
                FrequencyDisplayPlot { \
                    qproperty-marker_lower_intensity_color: white; \
                        qproperty-marker_upper_intensity_color: black; \
                        qproperty-marker_lower_intensity_visible: false; \
                        qproperty-marker_upper_intensity_visible: true; \
                        qproperty-marker_noise_floor_amplitude_color: red; \
                    qproperty-marker_noise_floor_amplitude_visible: true; \
            } \
                ");
    fft_gui->enable_rf_freq(true);

    QThread *t4 = new QThread;
    t4->setObjectName("radioop");
    RadioOp *radio_op = new RadioOp(settings, fft_gui,const_gui, rssi_gui);
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
    QObject::connect(w,SIGNAL(startAutoTuneFreq()),radio_op,SLOT(startAutoTune()));
    QObject::connect(w,SIGNAL(stopAutoTuneFreq()),radio_op,SLOT(stopAutoTune()));
    QObject::connect(w,SIGNAL(fineTuneFreq(long)),radio_op,SLOT(fineTuneFreq(long)));
    QObject::connect(w,SIGNAL(setTxPower(int)),radio_op,SLOT(setTxPower(int)));
    QObject::connect(w,SIGNAL(setRxSensitivity(int)),radio_op,SLOT(setRxSensitivity(int)));
    QObject::connect(w,SIGNAL(setSquelch(int)),radio_op,SLOT(setSquelch(int)));
    QObject::connect(w,SIGNAL(setVolume(int)),radio_op,SLOT(setVolume(int)));
    QObject::connect(w,SIGNAL(setRxCTCSS(float)),radio_op,SLOT(setRxCTCSS(float)));
    QObject::connect(w,SIGNAL(setTxCTCSS(float)),radio_op,SLOT(setTxCTCSS(float)));
    QObject::connect(w,SIGNAL(enableGUIConst(bool)),radio_op,SLOT(enableGUIConst(bool)));
    QObject::connect(w,SIGNAL(enableGUIFFT(bool)),radio_op,SLOT(enableGUIFFT(bool)));
    QObject::connect(w,SIGNAL(usePTTForVOIP(bool)),radio_op,SLOT(usePTTForVOIP(bool)));
    QObject::connect(w,SIGNAL(setVOIPForwarding(bool)),radio_op,SLOT(setVOIPForwarding(bool)));
    QObject::connect(w,SIGNAL(toggleRepeat(bool)),radio_op,SLOT(toggleRepeat(bool)));
    QObject::connect(w,SIGNAL(stopRadio()),radio_op,SLOT(stop()));
    QObject::connect(radio_op, SIGNAL(printText(QString)), w, SLOT(displayText(QString)));
    QObject::connect(radio_op, SIGNAL(printCallsign(QString)), w, SLOT(displayCallsign(QString)));
    QObject::connect(radio_op, SIGNAL(videoImage(QImage)), w, SLOT(displayImage(QImage)));
    QObject::connect(radio_op, SIGNAL(displayReceiveStatus(bool)), w, SLOT(displayReceiveStatus(bool)));
    QObject::connect(radio_op, SIGNAL(displayTransmitStatus(bool)), w, SLOT(displayTransmitStatus(bool)));
    QObject::connect(radio_op, SIGNAL(displayDataReceiveStatus(bool)), w, SLOT(displayDataReceiveStatus(bool)));
    QObject::connect(radio_op, SIGNAL(freqFromGUI(long)), w, SLOT(updateFreqGUI(long)));
    QObject::connect(radio_op, SIGNAL(pingServer()), client, SLOT(pingServer()));
    QObject::connect(client, SIGNAL(pcmAudio(short*,int,quint64)), radio_op, SLOT(processVoipAudioFrame(short*, int, quint64)));
    QObject::connect(radio_op, SIGNAL(voipData(short*,int)), client, SLOT(processAudio(short*,int)));

    QObject::connect(client,SIGNAL(onlineStations(StationList)),w,SLOT(updateOnlineStations(StationList)));
    QObject::connect(w,SIGNAL(connectToServer(QString, unsigned)),client,SLOT(connectToServer(QString, unsigned)));
    QObject::connect(w,SIGNAL(disconnectFromServer()),client,SLOT(disconnectFromServer()));
    QObject::connect(w,SIGNAL(setMute(bool)),client,SLOT(setMute(bool)));

    w->show();
    w->showMaximized();
    w->activateWindow();
    w->raise();

    int ret = a.exec();

    client->disconnectFromServer();
    delete w;
    delete client;
    return ret;
}
