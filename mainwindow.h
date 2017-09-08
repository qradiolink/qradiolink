// Written by Adrian Musceac YO8RZZ at gmail dot com, started October 2013.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
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
#include <QFile>
#include <QFileDialog>
#include <QScrollBar>
#include "mumbleclient.h"

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
    void GUIstartTalkVOIP();
    void GUIstopTalkVOIP();
    void GUIsendText();
    void chooseFile();
    void displayText(QString text);
    void displayReceiveStatus(bool status);
    void displayTransmitStatus(bool status);
    void displayDataReceiveStatus(bool status);
    void updateOnlineStations(StationList stations);
    void toggleRXwin(bool value);
    void toggleTXwin(bool value);
    void tuneCenterFreq(int value);
    void tuneMainFreq();
    void toggleWideband(bool value);
    void toggleMode(int value);
    void setTxPowerDisplay(int);
signals:
    void startTransmission();
    void endTransmission();
    void connectVOIP(QString host, unsigned port);
    void disconnectVOIP();
    void sendText(QString text, bool repeat);
    void startTalkVOIP();
    void stopTalkVOIP();
    void toggleRX(bool value);
    void toggleTX(bool value);
    void tuneFreq(long center_freq);
    void fineTuneFreq(long center_freq);
    void toggleWidebandMode(bool value);
    void toggleModemMode(int value);
    void setTxPower(int value);
public:
    explicit MainWindow(MumbleClient *client, QWidget *parent = 0);
    ~MainWindow();
    QWidget* get_const_gui() {return _constellation_gui;}
    QWidget* get_rssi_gui() {return _rssi_gui;}


private:
    Ui::MainWindow *ui;
    MumbleClient *_mumble_client;
    bool _transmitting_radio;
    QWidget *_constellation_gui;
    QWidget *_rssi_gui;

};

#endif // MAINWINDOW_H
