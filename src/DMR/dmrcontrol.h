// Written by Adrian Musceac YO8RZZ , started Oct 2024.
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

#ifndef DMRCONTROL_H
#define DMRCONTROL_H

#include <QObject>
#include <vector>
#include "src/settings.h"
#include "src/audio/audioencoder.h"
#include "src/logger.h"
#include "src/DMR/dmrframe.h"
#include "src/DMR/dmrutils.h"
#include "src/MMDVM/DMRLC.h"
#include "src/MMDVM/DMRFullLC.h"
#include "src/MMDVM/DMRShortLC.h"
#include "src/MMDVM/DMRCSBK.h"
#include "src/MMDVM/DMRDataHeader.h"
#include "src/MMDVM/DMRDefines.h"
#include "src/MMDVM/DMREMB.h"
#include "src/MMDVM/DMREmbeddedData.h"
#include "src/MMDVM/Utils.h"

namespace RECEIVER_STATE
{
    enum RECEIVER_STATE
    {
        STATE_IDLE = 0,
        STATE_LATE_ENTRY = 1,
        STATE_AUDIO = 2,
        STATE_DATA = 3,
    };
}
namespace TRANSMITTER_STATE
{
    enum TRANSMITTER_STATE
    {
        STATE_IDLE = 0,
        STATE_ACTIVE = 1,
        STATE_ENDING = 2,
    };
}

namespace DMR_MODE
{
    enum DMR_MODE
    {
        DMR_MODE_REPEATER = 0,
        DMR_MODE_DMO = 1,
        DMR_MODE_TRUNKED = 2,
    };
}

namespace DMR_CALL_TYPE
{
    enum DMR_CALL_TYPE
    {
        GROUP_CALL = 0,
        PRIVATE_CALL = 1,
    };
}


class DMRControl : public QObject
{
    Q_OBJECT
public:
    explicit DMRControl(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~DMRControl();
    void addFrames(std::vector<DMRFrame> frames);
    uint8_t getColorCode() const;
    uint8_t getSlotNo() const;
    uint8_t addTxAudio(unsigned char* encoded);
    bool getTxAudio(DMRFrame &frame);
    void getVoiceHeader(std::vector<DMRFrame> &frames);
    void getVoiceTerminator(DMRFrame &frame);
    void getStartCSBK(std::vector<DMRFrame> &frames);
    void initVoiceTX();
    void stopVoiceTX();
    bool getTXStatus() const;
    void clearTxAudio();

signals:
    void digitalAudio(unsigned char *audiodata, int size);
    void talkerAlias(QString ta, bool html);
    void gpsInfo(QString gps_info, bool html);
    void headerReceived(QString info, bool html);
    void terminatorReceived(QString info, bool html);
    void endBeep();

private:
    bool processAudio(DMRFrame &frame);
    bool processCSBK(const DMRFrame &frame);
    bool processVoiceHeader(const DMRFrame &frame);
    bool processTerminator(const DMRFrame &frame);
    bool processDataHeader(const DMRFrame &frame);
    bool processDataBlock(const DMRFrame &frame);
    bool processEmbeddedData();
    bool processColorCode(const DMRFrame &frame);
    bool processTimeslot(const DMRFrame &frame);
    void setGPSInfo(float longitude, float latitude, std::string error);
    void processTalkerAlias();
    const Settings *_settings;
    Logger *_logger;
    uint8_t _color_code_RX = 0;
    uint8_t _color_code_TX = 0;
    uint8_t _timeslot_RX = 0;
    uint8_t _FN_RX = 0;
    uint8_t _FN_TX = 0;
    uint8_t _superframe_RX = 0;
    uint8_t _superframe_TX = 0;
    uint8_t _at = 0;
    uint8_t _slot_no = 1;
    uint8_t _lcss = 0;
    uint32_t _receiver_state;
    uint32_t _transmitter_state;
    uint32_t _dstId_RX = 0;
    uint32_t _srcId_RX = 0;
    uint8_t _FLCO_RX = FLCO_GROUP;
    FLCO _FLCO_TX = FLCO_GROUP;
    CDMRLC _lc_RX;
    CDMRLC _lc_TX;
    CDMREmbeddedData _embedded_data_RX;
    CDMREmbeddedData _embedded_data_TX;
    bool _talker_alias_received;
    unsigned int _ta_df;
    unsigned int _ta_dl;
    QByteArray _ta_data;
    QVector<unsigned char*> _tx_frames;
    unsigned char _ta_tx[27U];

};

#endif // DMRCONTROL_H
