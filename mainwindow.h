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
#include <QGraphicsScene>
#include <QGraphicsView>
#include "mumbleclient.h"
#include <math.h>
#include <complex>
#include "qtgui/freqctrl.h"
#include <libconfig.h++>
#include "settings.h"
#include "channel.h"
#include "qtgui/plotter.h"
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
    void displayText(QString text, bool html);
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
    void toggleVox(bool value);
    void toggleRepeater(bool value);
    void newChannel(Channel *chan);
    void channelState(QTreeWidgetItem *item, int k);
    void newFFTData(std::complex<float>* fft_data, int fftsize);
    void carrierOffsetChanged(qint64 freq, qint64 offset);
    void setFFTSize(int size);
    void setAveraging(int x);
    void showControls();
    void showConstellation();
    void setEnabledFFT(bool value);
    void setPeakDetect(bool value);
    void updateRSSI(float value);
    void updateConstellation(std::vector<std::complex<float>>* constellation_data);
    void newWaterfallFPS();


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


public:
    explicit MainWindow(Settings *settings, QWidget *parent = 0);
    ~MainWindow();

    void readConfig();
private:
    Ui::MainWindow *ui;
    bool _transmitting_radio;
    QPixmap *_video_img;
    QFileInfo *_config_file;
    qint64 _rx_frequency;
    qint64 _tx_frequency;
    QFileInfo *setupConfig();
    void closeEvent(QCloseEvent *);
    void resizeEvent(QResizeEvent *);
    Settings *_settings;
    int _current_voip_channel;
    float *_realFftData;
    float *_pwrFftData;
    float *_iirFftData;
    float _fft_averaging;
    int _waterfall_fps;
    qint64 _demod_offset;
    bool _show_controls;
    bool _show_constellation;
    bool _fft_enabled;
    QGraphicsScene *_scene;
    QGraphicsView *_view;
    void setFilterWidth(int index);
    std::vector<std::complex<int>> *_filter_widths;



};

#endif // MAINWINDOW_H
