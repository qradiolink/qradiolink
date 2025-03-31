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

#include "dmrframe.h"
#include <iostream>


DMRFrame::DMRFrame(uint8_t type)
{
    memset(_cach_data, 0, CACH_LENGTH_BYTES);
    memset(_frame_data, 0, FRAME_LENGTH_BYTES);
    _frame_type = type;
    _translation_map.push_back(2);
    _translation_map.push_back(3);
    _translation_map.push_back(1);
    _translation_map.push_back(0);

    _symbol_map.push_back(-1.0f);
    _symbol_map.push_back(-0.33333f);
    _symbol_map.push_back(0.33333f);
    _symbol_map.push_back(1.0f);
}

DMRFrame::DMRFrame(uint8_t *bytes, uint8_t type)
{
    memset(_cach_data, 0, CACH_LENGTH_BYTES);
    memcpy(_frame_data, bytes, FRAME_LENGTH_BYTES * sizeof(uint8_t));
    _frame_type = type;
    if(_frame_type == DMRFrameType::DMRFrameTypeData)
    {
        CDMRSlotType slot_type;
        slot_type.putData(_frame_data);
        _color_code = slot_type.getColorCode();
        _data_type = slot_type.getDataType();
    }
    else if(_frame_type == DMRFrameType::DMRFrameTypeVoice)
    {
        _data_type = DT_VOICE;
    }
    else if(_frame_type == DMRFrameType::DMRFrameTypeVoiceSync)
    {
        _data_type = DT_VOICE_SYNC;
    }
}

DMRFrame::DMRFrame(std::vector<uint8_t> bits, uint8_t type)
{
    memset(_cach_data, 0, CACH_LENGTH_BYTES);
    memset(_frame_data, 0, FRAME_LENGTH_BYTES);
    packFrame(_cach_data, bits, 0, CACH_LENGTH_BITS);
    packFrame(_frame_data, bits, CACH_LENGTH_BITS, CACH_LENGTH_BITS + FRAME_LENGTH_BITS);
    _frame_type = type;
    if(_frame_type == DMRFrameType::DMRFrameTypeData)
    {
        CDMRSlotType slot_type;
        slot_type.putData(_frame_data);
        _color_code = slot_type.getColorCode();
        _data_type = slot_type.getDataType();
    }
    else if(_frame_type == DMRFrameType::DMRFrameTypeVoice)
    {
        _data_type = DT_VOICE;
    }
    else if(_frame_type == DMRFrameType::DMRFrameTypeVoiceSync)
    {
        _data_type = DT_VOICE_SYNC;
    }
}

DMRFrame::~DMRFrame()
{
    _translation_map.clear();
    _symbol_map.clear();
}

std::vector<uint8_t> DMRFrame::toByteVector()
{
    std::vector<uint8_t> bytes;
    for(uint32_t i=0;i<FRAME_LENGTH_BYTES;i++)
    {
        bytes.push_back(_frame_data[i]);
    }
    return bytes;
}

std::vector<float> DMRFrame::toSymbolVector()
{
    std::vector<float> symbols;
    for(uint32_t i=0;i<FRAME_LENGTH_BYTES;i++)
    {
        uint8_t byte = _frame_data[i];
        for(int8_t j=3;j>=0;j--)
        {
            uint8_t dibit = (byte >> (j*2)) & 0x03;
            uint8_t t = _translation_map.at(dibit);
            float symbol = _symbol_map.at(t);
            symbols.push_back(symbol);
        }
    }
    return symbols;
}

bool DMRFrame::validate()
{
    if(_frame_type == DMRFrameType::DMRFrameTypeData)
    {
        switch(_data_type)
        {
            case DT_CSBK:
            {
                CDMRCSBK csbk;
                bool valid = csbk.put(_frame_data);
                if (!valid)
                    return false;
                csbk.get(_frame_data);
                _srcId = csbk.getSrcId();
                _dstId = csbk.getDstId();
                _FLCO = csbk.getCSBKO();
                _FID = csbk.getFID();
                break;
            }
            case DT_MBC_HEADER:
            {
                CDMRCSBK csbk;
                csbk.setDataType(DT_MBC_HEADER);
                bool valid = csbk.put(_frame_data);
                if (!valid)
                    return false;
                csbk.get(_frame_data);
                _srcId = csbk.getSrcId();
                _dstId = csbk.getDstId();
                _FLCO = csbk.getCSBKO();
                _FID = csbk.getFID();
                break;
            }
            case DT_MBC_CONTINUATION:
            {
                CDMRCSBK csbk;
                csbk.setDataType(DT_MBC_CONTINUATION);
                bool valid = csbk.put(_frame_data);
                if (!valid)
                    return false;
                csbk.get(_frame_data);
                _srcId = csbk.getSrcId();
                _dstId = csbk.getDstId();
                _FLCO = csbk.getCSBKO();
                _FID = csbk.getFID();
                break;
            }
            case DT_DATA_HEADER:
            {
                CDMRDataHeader header;
                bool valid = header.put(_frame_data);
                if (!valid)
                    return false;
                header.get(_frame_data);
                _srcId = header.getSrcId();
                _dstId = header.getDstId();
                _FLCO = header.getOpcode();
                break;
            }
            case DT_VOICE_LC_HEADER:
            {
                CDMRFullLC fullLC;
                CDMRLC* lc = fullLC.decode(_frame_data, DT_VOICE_LC_HEADER);
                if (lc == NULL)
                     return false;
                fullLC.encode(lc, _frame_data, DT_VOICE_LC_HEADER);
                _srcId = lc->getSrcId();
                _dstId = lc->getDstId();
                _FLCO = lc->getFLCO();
                _FID = lc->getFID();
                delete lc;
                break;
            }
            case DT_VOICE_PI_HEADER:
            {
                CBPTC19696 bptc;
                unsigned char payload[12U];
                bptc.decode(_frame_data, payload);
                bptc.encode(payload, _frame_data);
                break;
            }
            case DT_TERMINATOR_WITH_LC:
            {
                CDMRFullLC fullLC;
                CDMRLC* lc = fullLC.decode(_frame_data, DT_TERMINATOR_WITH_LC);
                if (lc == NULL)
                     return false;
                fullLC.encode(lc, _frame_data, DT_TERMINATOR_WITH_LC);
                _srcId = lc->getSrcId();
                _dstId = lc->getDstId();
                _FLCO = lc->getFLCO();
                _FID = lc->getFID();
                delete lc;
                break;
            }
            case DT_RATE_12_DATA:
            {
                CBPTC19696 bptc;
                unsigned char payload[12U];
                bptc.decode(_frame_data, payload);
                bptc.encode(payload, _frame_data);
                break;
            }
            case DT_RATE_34_DATA:
            {
                CDMRTrellis trellis;
                unsigned char payload[18U];
                bool ret = trellis.decode(_frame_data, payload);
                if (ret)
                {
                    trellis.encode(payload, _frame_data);
                } else
                {
                    return false;
                }
                break;
            }
            case DT_RATE_1_DATA:
            {
                break;
            }
            case DT_IDLE:
            {
                break;
            }
            default:
            {
                return false;
                break;
            }
        }
    }
    else if(_frame_type == DMRFrameType::DMRFrameTypeVoice)
    {
    }
    else if(_frame_type == DMRFrameType::DMRFrameTypeVoiceSync)
    {
    }
    return true;
}

int DMRFrame::runAudioFEC()
{
    CAMBEFEC fec;
    int errors = fec.regenerateDMR(_frame_data);
    //if(errors > 0)
    //    qDebug() << " Audio frame errors: " << errors ;
    return errors;
}

bool DMRFrame::setDownlink(bool dl)
{
    _downlink = dl;
    if(dl)
    {
        uint8_t tc, at, ls1, ls0;
        at = (_cach_data[0] >> 7) & 0x1;
        tc = (_cach_data[0] >> 3) & 0x1;
        ls1 = (_cach_data[1] >> 7) & 0x1;
        ls0 = (_cach_data[1] >> 3) & 0x1;
        bool h0 = at ^ tc ^ ls1;
        bool h1 =      tc ^ ls1 ^ ls0;
        bool h2 = at ^ tc       ^ ls0;

        bool h0_c = (_cach_data[1] >> 1) & 0x1;
        bool h1_c = (_cach_data[2] >> 5) & 0x1;
        bool h2_c = (_cach_data[2] >> 1) & 0x1;
        if(!((h0 == h0_c) && (h1 == h1_c) && (h2 == h2_c)))
        {
            _cach_decoded = false;
            return false;
        }
        else
        {
            _at = at;
            _slot_no = tc ? 2 : 1;
            _lcss = (ls1 << 1) | ls0;
            _cach_decoded = true;
            return true;
        }
    }
    // If not a downlink signal, CACH info is not present
    return false;
}

bool DMRFrame::cachDecoded() const
{
    return _cach_decoded;
}

void DMRFrame::getCACH(unsigned char * cach_data) const
{
    memcpy(cach_data, _cach_data, CACH_LENGTH_BYTES);
}

void DMRFrame::setFN(uint8_t fn)
{
    _FN = fn;
}

uint8_t DMRFrame::getFN() const
{
    return _FN;
}

uint8_t DMRFrame::getFrameType() const
{
    return _frame_type;
}

uint8_t DMRFrame::getDataType() const
{
    return _data_type;
}

void DMRFrame::setDataType(uint8_t data_type)
{
    _data_type = data_type;
}

void DMRFrame::setFrameType(uint8_t frame_type)
{
    _frame_type = frame_type;
}

uint8_t DMRFrame::getColorCode() const
{
    return _color_code;
}

uint8_t DMRFrame::getSlotNo() const
{
    return _slot_no;
}

uint32_t DMRFrame::getSrcId() const
{
    return _srcId;
}

uint32_t DMRFrame::getDstId() const
{
    return _dstId;
}

uint8_t DMRFrame::getFLCO() const
{
    return _FLCO;
}

uint8_t DMRFrame::getFID() const
{
    return _FID;
}

uint64_t DMRFrame::getTransmitTimestamp() const
{
    return _transmit_timestamp;
}

void DMRFrame::setTransmitTimestamp(uint64_t ts)
{
    _transmit_timestamp = ts;
}

void DMRFrame::packFrame(unsigned char *pktbuf, std::vector<uint8_t> bitbuf, unsigned int start, unsigned int bitcount)
{
    for(unsigned int i = start; i < bitcount; i += 8)
    {
        int t = bitbuf[i+0] & 0x1;
        t = (t << 1) | (bitbuf[i+1] & 0x1);
        t = (t << 1) | (bitbuf[i+2] & 0x1);
        t = (t << 1) | (bitbuf[i+3] & 0x1);
        t = (t << 1) | (bitbuf[i+4] & 0x1);
        t = (t << 1) | (bitbuf[i+5] & 0x1);
        t = (t << 1) | (bitbuf[i+6] & 0x1);
        t = (t << 1) | (bitbuf[i+7] & 0x1);
        *pktbuf++ = t;
    }
}

bool DMRFrame::getVoice(unsigned char *data) const
{
    if((_frame_type == DMRFrameType::DMRFrameTypeVoice) || (_frame_type == DMRFrameType::DMRFrameTypeVoiceSync))
    {
        memcpy(data, _frame_data, 14);
        data[13] &= 0xF0;
        data[13] |= (_frame_data[19] & 0x0F);
        memcpy(data + 14, &_frame_data[20], 13);
        return true;
    }
    return false;
}

bool DMRFrame::setVoice(unsigned char *voice_data)
{
    if((_frame_type == DMRFrameType::DMRFrameTypeVoice) || (_frame_type == DMRFrameType::DMRFrameTypeVoiceSync))
    {
        memcpy(_frame_data, voice_data, 14);
        _frame_data[13] &= 0xF0;
        _frame_data[19] = 0x00;
        _frame_data[19] |= (voice_data[13] & 0x0F);
        memcpy(&_frame_data[20], &voice_data[14], 13);
        return true;
    }
    return false;
}

void DMRFrame::setEmbeddedData(CDMREmbeddedData &embedded_data, uint8_t fn)
{
    unsigned char lcss = 0x0;
    CDMREMB emb;
    lcss = embedded_data.getData(_frame_data, fn);
    emb.setColorCode(_color_code);
    emb.setLCSS(lcss);
    emb.getData(_frame_data);
}

void DMRFrame::getData(unsigned char *data) const
{
    memcpy(data, _frame_data, FRAME_LENGTH_BYTES);
}

void DMRFrame::getDataPayload(unsigned char *data, uint8_t &length) const
{
    switch(_frame_type)
    {
        case DT_RATE_12_DATA:
        {
            CBPTC19696 bptc;
            length = 12U;
            unsigned char payload[12U];
            bptc.decode(_frame_data, payload);
            memcpy(data, payload, 12U);
            break;
        }
        case DT_RATE_34_DATA:
        {
            CDMRTrellis trellis;
            length = 18U;
            unsigned char payload[18U];
            trellis.decode(_frame_data, payload);
            memcpy(data, payload, 18U);
            break;
        }
        case DT_RATE_1_DATA:
        {
            length = 24U;
            memcpy(data, _frame_data, 24U);
            break;
        }
    }
}


void DMRFrame::constructCSBKFrame(CDMRCSBK &csbk)
{
    unsigned char repacked_data[DMR_FRAME_LENGTH_BYTES];
    // Regenerate the CSBK data
    csbk.setDataType(DT_CSBK);
    csbk.get(repacked_data);
    CDMRSlotType slotType;
    slotType.setColorCode(_color_code);
    slotType.setDataType(_data_type);
    slotType.getData(repacked_data);

    CSync::addDMRDataSync(repacked_data, false);
    memcpy(_frame_data, repacked_data, DMR_FRAME_LENGTH_BYTES);
}

void DMRFrame::constructLCFrame(CDMRLC *lc)
{
    unsigned char repacked_data[DMR_FRAME_LENGTH_BYTES];

    CDMRFullLC fullLC;
    fullLC.encode(lc, repacked_data, _data_type);
    CDMRSlotType slotType;
    slotType.setColorCode(_color_code);
    slotType.setDataType(_data_type);
    slotType.getData(repacked_data);
    CSync::addDMRDataSync(repacked_data, false);
    memcpy(_frame_data, repacked_data, DMR_FRAME_LENGTH_BYTES);
}

void DMRFrame::setColorCode(uint8_t color_code)
{
    _color_code = color_code;
}
void DMRFrame::setSlotNo(uint8_t slot_no)
{
    _slot_no = slot_no;
}

void DMRFrame::addVoiceSync()
{
    CSync::addDMRAudioSync(_frame_data, false);
}
