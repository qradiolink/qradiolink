// Written by Adrian Musceac YO8RZZ , started October 2013.
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QScrollBar>
#include <QDir>
#include <QFileInfo>
#include <QTreeWidgetItem>

#include "mumbleclient.h"
#include <math.h>
#include "qtgui/freqctrl.h"
#include <libconfig.h++>
#include "settings.h"
#include "channel.h"
#include <iostream>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public slots:
    void GUIstartTransmission();
    void GUIendTransmission();
    void GUIconnectVOIP();
    void GUIdisconnectVOIP();
    void GUIsendText();
    void chooseFile();
    void displayText(QString text);
    void displayCallsign(QString callsign);
    void displayReceiveStatus(bool status);
    void displayTransmitStatus(bool status);
    void displayDataReceiveStatus(bool status);
    void updateOnlineStations(StationList stations);
    void toggleRXwin(bool value);
    void toggleTXwin(bool value);
    void tuneCenterFreq(int value);
    void tuneMainFreq(qint64 freq);
    void toggleWideband(bool value);
    void toggleRxMode(int value);
    void toggleTxMode(int value);
    void setTxPowerDisplay(int value);
    void setRxSensitivityDisplay(int value);
    void setSquelchDisplay(int value);
    void setVolumeDisplay(int value);
    void autoTune(bool value);
    void displayImage(QImage img);
    void enterFreq();
    void saveConfig();
    void mainTabChanged(int value);
    void clearTextArea();
    void updateFreqGUI(long freq);
    void enterShift();
    void updateRxCTCSS(int value);
    void updateTxCTCSS(int value);
    void togglePTTVOIP(bool value);
    void toggleVOIPForwarding(bool value);
    void toggleRepeater(bool value);
    void newChannel(Channel *chan);
    void channelState(QTreeWidgetItem *item, int k);

signals:
    void startTransmission();
    void endTransmission();
    void sendText(QString text, bool repeat);
    void toggleRX(bool value);
    void toggleTX(bool value);
    void tuneFreq(qint64 center_freq);
    void tuneTxFreq(qint64 center_freq);
    void fineTuneFreq(long center_freq);
    void toggleWidebandMode(bool value);
    void toggleRxModemMode(int value);
    void toggleTxModemMode(int value);
    void setTxPower(int value);
    void setRxSensitivity(int value);
    void setSquelch(int value);
    void setVolume(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void enableGUIConst(bool value);
    void enableGUIFFT(bool value);
    void startAutoTuneFreq();
    void stopAutoTuneFreq();
    void usePTTForVOIP(bool value);
    void setVOIPForwarding(bool value);
    void connectToServer(QString server, unsigned port);
    void disconnectFromServer();
    void changeChannel(int id);
    void setMute(bool value);
    void toggleRepeat(bool value);
    void stopRadio();


public:
    explicit MainWindow(Settings *settings, QWidget *parent = 0);
    ~MainWindow();
    QWidget* get_const_gui() {return _constellation_gui;}
    QWidget* get_rssi_gui() {return _rssi_gui;}
    QWidget* get_fft_gui() {return _fft_gui;}

    void readConfig();
private:
    Ui::MainWindow *ui;
    bool _transmitting_radio;
    QWidget *_constellation_gui;
    QWidget *_rssi_gui;
    QWidget *_fft_gui;
    QPixmap *_video_img;
    QFileInfo *_config_file;
    qint64 _rx_frequency;
    qint64 _tx_frequency;
    QFileInfo *setupConfig();
    void closeEvent(QCloseEvent *);
    Settings *_settings;
    int _current_voip_channel;



};

#endif // MAINWINDOW_H
