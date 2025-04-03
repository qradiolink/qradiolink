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

#ifndef DMRFRAME_H
#define DMRFRAME_H

#include <vector>
#include <string.h>
#include "src/DMR/constants.h"
#include "src/MMDVM/DMRData.h"
#include "src/MMDVM/DMRSlotType.h"
#include "src/MMDVM/DMRDefines.h"
#include "src/MMDVM/DMRCSBK.h"
#include "src/MMDVM/DMRDataHeader.h"
#include "src/MMDVM/DMRFullLC.h"
#include "src/MMDVM/DMRTrellis.h"
#include "src/MMDVM/DMREmbeddedData.h"
#include "src/MMDVM/DMREMB.h"
#include "src/MMDVM/AMBEFEC.h"
#include "src/MMDVM/Sync.h"

enum DMRFrameType
{
    DMRFrameTypeData = 0,
    DMRFrameTypeVoice = 1,
    DMRFrameTypeVoiceSync = 2,
};

enum RECV_STATE
{
    RECV_NONE = 0,
    RECV_DATA = 1,
    RECV_VOICE_SYNC = 2,
    RECV_VOICE = 3,
};

class DMRFrame
{
public:
    DMRFrame(uint8_t type);
    DMRFrame(std::vector<uint8_t> bits, uint8_t type);
    DMRFrame(uint8_t *bytes, uint8_t type);
    ~DMRFrame();
    void constructCSBKFrame(CDMRCSBK &csbk);
    void constructLCFrame(CDMRLC *lc);
    std::vector<uint8_t> toByteVector();
    std::vector<float> toSymbolVector();
    bool validate();
    void setFN(uint8_t fn);
    bool setDownlink(bool dl);
    uint8_t getFN() const;
    uint8_t getFrameType() const;
    uint8_t getDataType() const;
    uint8_t getColorCode() const;
    uint8_t getSlotNo() const;
    uint64_t getTransmitTimestamp() const;
    void setTransmitTimestamp(uint64_t ts);
    void setColorCode(uint8_t color_code);
    void setSlotNo(uint8_t slot_no);
    bool getVoice(unsigned char *data) const;
    bool setVoice(unsigned char* voice_data);
    void setEmbeddedData(CDMREmbeddedData &embedded_data, uint8_t fn);
    void getData(unsigned char *data) const;
    void getDataPayload(unsigned char *data, uint8_t &length) const;
    void getCACH(unsigned char *cach_data) const;
    bool cachDecoded() const;
    void addVoiceSync();
    int runAudioFEC();
    void setDataType(uint8_t data_type);
    void setFrameType(uint8_t frame_type);
    uint32_t getSrcId() const;
    uint32_t getDstId() const;
    uint8_t getFLCO() const;
    uint8_t getFID() const;



private:
    void packFrame(unsigned char *pktbuf, std::vector<uint8_t> bitbuf, unsigned int start, unsigned int bitcount);

    unsigned char _cach_data[CACH_LENGTH_BYTES];
    unsigned char _frame_data[FRAME_LENGTH_BYTES];
    std::vector<uint8_t> _translation_map;
    std::vector<float> _symbol_map;
    uint8_t _frame_type = DMRFrameTypeData;
    uint8_t _data_type = DT_IDLE;
    uint8_t _color_code = 255;
    uint8_t _FN = 0;
    uint8_t _at = 0;
    uint8_t _slot_no = 0;
    uint8_t _lcss = 0;
    uint32_t _srcId = 0;
    uint32_t _dstId = 0;
    uint8_t _FLCO = 0;
    uint8_t _FID = 0;
    bool _downlink;
    bool _cach_decoded = false;
    uint64_t _receive_timestamp;
    uint64_t _transmit_timestamp;
};

#endif // DMRFRAME_H
