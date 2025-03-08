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


#ifndef GR_MODEM_H
#define GR_MODEM_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QtEndian>
#include <QMutex>
#include <QByteArray>
#include <QCoreApplication>
#include <string>
#include "src/ext/utils.h"
#include "src/settings.h"
#include "src/limits.h"
#include "src/logger.h"
#include "src/layer1framing.h"
#include "src/modem_types.h"
#include "src/gr/gr_mod_base.h"
#include "src/gr/gr_demod_base.h"
#include "src/bursttimer.h"

/// M17 code
#include <M17/M17FrameDecoder.hpp>
#include <M17/M17FrameEncoder.hpp>
#include <M17/M17Transmitter.hpp>


#include <math.h>

class gr_modem : public QObject
{
    Q_OBJECT
public:

    explicit gr_modem(const Settings *settings, Logger *logger, QObject *parent = 0);
    ~gr_modem();

    bool demodulateAnalog();
    void sendCallsign(QString callsign);

signals:
    void pcmAudio(std::vector<float>* pcm);
    void digitalAudio(unsigned char *c2data, int size);
    void videoData(unsigned char *video_data, int size);
    void netData(unsigned char *net_data, int size);
    void demodulated_audio(short *pcm, short size);
    void textReceived(QString text);
    void protoReceived(QByteArray data);
    void callsignReceived(QString text);
    void m17FrameInfoReceived(QString src, QString dest, uint16_t CAN);
    void audioFrameReceived();
    void dataFrameReceived();
    void syncIssues();
    void receiveEnd();
    void endAudioTransmission();

public slots:
    void transmitPCMAudio(std::vector<float> *audio_data);
    void transmitDigitalAudio(unsigned char *data, int size);
    void transmitM17Audio(unsigned char *data, int size);
    void transmitVideoData(unsigned char *data, int size);
    void transmitNetData(unsigned char *data, int size);
    bool demodulate();
    void startTransmission(QString callsign);
    void endTransmission(QString callsign);
    void transmitTextData(QString text, int frame_type = FrameTypeText);
    void transmitBinData(QByteArray bin_data, int frame_type = FrameTypeProto);
    void initTX(int modem_type, int64_t frequency, std::string device_args,
                std::string device_antenna, int freq_corr, int initial_gain=94, int mmdvm_channels=3,
                int mmdvm_channel_separation=25000);
    void initRX(int modem_type, std::string device_args,
                std::string device_antenna, int freq_corr, int mmdvm_channels=3,
                int mmdvm_channel_separation=25000);
    void deinitTX(int modem_type);
    void deinitRX(int modem_type);
    void toggleRxMode(int modem_type);
    void toggleTxMode(int modem_type);
    void tune(int64_t center_freq);
    void tuneTx(int64_t center_freq);
    void startRX(int buffer_size=0);
    void stopRX();
    void startTX(int buffer_size=0);
    void stopTX();
    void setTxPower(float value, std::string gain_stage="");
    void setBbGain(int value);
    void setGain(int value);
    void setK(bool value);
    void setSquelch(int value);
    void setFilterWidth(int filter_width);
    void setRxSensitivity(double value, std::string gain_stage="");
    void setAgcAttack(int value);
    void setAgcDecay(int value);
    void setRxCTCSS(float value);
    void setTxCTCSS(float value);
    void enableGUIConst(bool value);
    void enableGUIFFT(bool value);
    void enableTimeDomain(bool value);
    void enableRSSI(bool value);
    void calibrateRSSI(float value);
    void enableDemod(bool value);
    double getFreqGUI();
    void getFFTData(float *data, unsigned int &size);
    void getSampleData(float *data, unsigned int &size);
    void setSampleWindow(unsigned int size);
    void setTimeDomainSampleRate(unsigned int samp_rate);
    void setTimeDomainFilterWidth(double filter_width);
    void setCarrierOffset(int64_t offset);
    void setTxCarrierOffset(int64_t offset);
    qint64 resetTxCarrierOffset();
    void setSampRate(int samp_rate);
    void setFFTSize(int size);
    float getRSSI();
    void flushSources();
    std::vector<gr_complex> *getConstellation();
    const QMap<std::string, QVector<int> > getRxGainNames() const;
    const QMap<std::string, QVector<int> > getTxGainNames() const;

private:
    std::vector<unsigned char>* frame(unsigned char *encoded_audio,
                                      int data_size, int frame_type=FrameTypeVoice);
    void processReceivedData(unsigned char* received_data, uint64_t current_frame_type);
    void handleStreamEnd();
    int findSync(unsigned char bit);
    void transmit(QVector<std::vector<unsigned char>*> frames);
    bool synchronize(int v_size, std::vector<unsigned char> *data);

    const Settings *_settings;
    Logger *_logger;
    Limits *_limits;
    gr_mod_base *_gr_mod_base;
    gr_demod_base *_gr_demod_base;
    unsigned char *_bit_buf;

    int _bit_buf_index;
    int _bit_buf_len;
    int _modem_type_rx;
    int _modem_type_tx;
    int _tx_frame_length;
    int _rx_frame_length;
    quint64 _frame_counter;
    int _last_frame_type;
    bool _sync_found;
    uint64_t _current_frame_type;
    uint64_t _shift_reg;
    int _modem_sync;
    BurstTimer *_burst_timer;

    /// M17 code
    M17::M17FrameDecoder _m17_decoder;      ///< M17 frame decoder
    M17::M17FrameEncoder _m17_encoder;      ///< M17 frame encoder
    M17::M17Transmitter  _m17_transmitter;        ///< M17 transmission manager.
    bool _m17_decoder_locked;

};

#endif // GR_MODEM_H
