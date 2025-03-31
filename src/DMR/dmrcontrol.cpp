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

#include "dmrcontrol.h"

DMRControl::DMRControl(const Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _receiver_state = RECEIVER_STATE::STATE_IDLE;
    _transmitter_state = TRANSMITTER_STATE::STATE_IDLE;
    _talker_alias_received = false;
    _FN_TX = 0;
    _superframe_TX = 0;
    _color_code_RX = 0;
    _FLCO_TX = (_settings->dmr_call_type == DMR_CALL_TYPE::GROUP_CALL) ? FLCO_GROUP : FLCO_USER_USER;
    _lc_TX = CDMRLC(_FLCO_TX, _settings->dmr_source_id, _settings->dmr_destination_id);
    if(!_settings->dmr_vocoder)
        _lc_TX.setFID(0xC2); // For Codec2 voice
    _embedded_data_TX.setLC(_lc_TX);
    memset(_ta_tx, 0, 27U);
    unsigned int size = std::min(_settings->dmr_talker_alias.length(), 27);
    memcpy(_ta_tx, _settings->dmr_talker_alias.toStdString().c_str(), size * sizeof(unsigned char));
}

DMRControl::~DMRControl()
{

}

void DMRControl::initVoiceTX()
{
    _FN_TX = 0;
    _superframe_TX = 0;
    _transmitter_state = TRANSMITTER_STATE::STATE_ACTIVE;

    memset(_ta_tx, 0, 27U);
    unsigned int size = std::min(_settings->dmr_talker_alias.length(), 27);
    memcpy(_ta_tx, _settings->dmr_talker_alias.toStdString().c_str(), size * sizeof(unsigned char));
}

void DMRControl::stopVoiceTX()
{
    _transmitter_state = TRANSMITTER_STATE::STATE_ENDING;
}

bool DMRControl::getTXStatus() const
{
    return (_transmitter_state != TRANSMITTER_STATE::STATE_IDLE);
}

uint8_t DMRControl::addTxAudio(unsigned char* encoded)
{
    _tx_frames.append(encoded);
    return _tx_frames.size();
}

void DMRControl::clearTxAudio()
{
    for(int i=0;i<_tx_frames.size();i++)
    {
        delete[] _tx_frames[i];
    }
    _tx_frames.clear();
}

void DMRControl::getVoiceHeader(std::vector<DMRFrame> &frames)
{
    // LC frames
    _FLCO_TX = (_settings->dmr_call_type == DMR_CALL_TYPE::GROUP_CALL) ? FLCO_GROUP : FLCO_USER_USER;
    _lc_TX = CDMRLC(_FLCO_TX, _settings->dmr_source_id, _settings->dmr_destination_id);
    if(!_settings->dmr_vocoder)
        _lc_TX.setFID(0xC2); // For Codec2 voice
    _embedded_data_TX.setLC(_lc_TX);
    DMRFrame frame_lc(DMRFrameTypeData);
    frame_lc.setDataType(DT_VOICE_LC_HEADER);
    frame_lc.setColorCode(_settings->dmr_color_code);
    frame_lc.setSlotNo(_settings->dmr_timeslot);
    frame_lc.constructLCFrame(&_lc_TX);
    for(int i=0;i<2;i++)
    {
        frames.push_back(frame_lc);
    }
}

void DMRControl::getStartCSBK(std::vector<DMRFrame> &frames)
{
    CDMRCSBK csbk;
    csbk.setCSBKO(CSBKO_BSDWNACT);
    csbk.setCBF(0x00);
    csbk.setData1(0x00);
    csbk.setSrcId(_settings->dmr_source_id);
    csbk.setDstId(_settings->dmr_destination_id);
    DMRFrame frame(DMRFrameTypeData);
    frame.setDataType(DT_CSBK);
    frame.setColorCode(_settings->dmr_color_code);
    frame.setSlotNo(_settings->dmr_timeslot);
    frame.constructCSBKFrame(csbk);
    for(int i=0;i<3;i++)
    {
        frames.push_back(frame);
    }
}

void DMRControl::getVoiceTerminator(DMRFrame &frame)
{
    CDMRLC lc(_FLCO_TX, _settings->dmr_source_id, _settings->dmr_destination_id);
    frame.setFrameType(DMRFrameTypeData);
    frame.setDataType(DT_TERMINATOR_WITH_LC);
    frame.setColorCode(_settings->dmr_color_code);
    frame.setSlotNo(_settings->dmr_timeslot);
    frame.constructLCFrame(&lc);
}

bool DMRControl::getTxAudio(DMRFrame &frame)
{
    if((_transmitter_state == TRANSMITTER_STATE::STATE_ENDING) && (_FN_TX == 0))
    {
        getVoiceTerminator(frame);
        clearTxAudio();
        _transmitter_state = TRANSMITTER_STATE::STATE_IDLE;
        _superframe_TX = 0;
        return true;
    }
    if(_tx_frames.size() < 3)
        return false;
    unsigned char audio_data[27];
    unsigned char *audio_frame1 = _tx_frames[0];
    unsigned char *audio_frame2 = _tx_frames[1];
    unsigned char *audio_frame3 = _tx_frames[2];
    ::memcpy(audio_data, audio_frame1, 9U);
    ::memcpy(audio_data + 9U, audio_frame2, 9U);
    ::memcpy(audio_data + 18U, audio_frame3, 9U);
    frame.setColorCode(_settings->dmr_color_code);
    frame.setSlotNo(_settings->dmr_timeslot);
    if(_FN_TX == 0)
    {
        frame.setFrameType(DMRFrameType::DMRFrameTypeVoiceSync);
        frame.setVoice(audio_data);
        frame.addVoiceSync();
    }
    else
    {
        frame.setFrameType(DMRFrameType::DMRFrameTypeVoice);
        frame.setVoice(audio_data); 
        frame.setEmbeddedData(_embedded_data_TX, _FN_TX);
    }
    delete[] audio_frame1;
    delete[] audio_frame2;
    delete[] audio_frame3;
    _tx_frames.remove(0, 3);

    // Frame and superframe handling
    _FN_TX++;
    if(_FN_TX > 5)
    {
        _FN_TX = 0;
        _superframe_TX++;
        if(_superframe_TX > 4)
        {
            _superframe_TX = 0;
        }
        if(_superframe_TX == 0)
        {
            _embedded_data_TX.setLC(_lc_TX);
        }
        else
        {
            FLCO flco;
            switch (_superframe_TX) {
            case 1:
                flco = FLCO_TALKER_ALIAS_HEADER;
                break;
            case 2:
                flco = FLCO_TALKER_ALIAS_BLOCK1;
                break;
            case 3:
                flco = FLCO_TALKER_ALIAS_BLOCK2;
                break;
            case 4:
                flco = FLCO_TALKER_ALIAS_BLOCK3;
                break;
            default:
                flco = FLCO_TALKER_ALIAS_HEADER;
                break;
            }
            unsigned int i = (_superframe_TX - 1) * 6;
            if(_superframe_TX == 1)
            {
                unsigned char options = (1 << 6) | (0x1B << 1);
                unsigned int a1 = ((unsigned int)_ta_tx[i] << 16) | ((unsigned int)_ta_tx[i + 1] << 8) | ((unsigned int)_ta_tx[i + 2]);
                unsigned int a2 = ((unsigned int)_ta_tx[i + 3] << 16) | ((unsigned int)_ta_tx[i + 4] << 8) | ((unsigned int)_ta_tx[i + 5]);
                CDMRLC ta_lc(flco, a2, a1);
                ta_lc.setOptions(options);
                _embedded_data_TX.setLC(ta_lc);
            }
            else
            {
                unsigned char options = _ta_tx[i];
                unsigned int a1 = ((unsigned int)_ta_tx[i + 1] << 16) | ((unsigned int)_ta_tx[i + 2] << 8) | ((unsigned int)_ta_tx[i + 3]);
                unsigned int a2 = ((unsigned int)_ta_tx[i + 4] << 16) | ((unsigned int)_ta_tx[i + 5] << 8) | ((unsigned int)_ta_tx[i + 6]);
                CDMRLC ta_lc(flco, a2, a1);
                ta_lc.setOptions(options);
                _embedded_data_TX.setLC(ta_lc);
            }
        }
    }
    return true;
}

bool DMRControl::processAudio(DMRFrame &frame)
{
    if(!processTimeslot(frame))
        return false;
    if(!processColorCode(frame))
        return false;
    if(_settings->dmr_vocoder)
    {
        int errors = frame.runAudioFEC();
    }
    uint8_t data_type= frame.getDataType();
    if(data_type == DT_VOICE_SYNC)
    {
        _embedded_data_RX.reset();

        if(_receiver_state == RECEIVER_STATE::STATE_IDLE)
            _receiver_state = RECEIVER_STATE::STATE_LATE_ENTRY;
    }
    else if(data_type == DT_VOICE)
    {
        unsigned char data[FRAME_LENGTH_BYTES];
        frame.getData(data);
        CDMREMB emb;
        emb.putData(data);
        unsigned char lcss = emb.getLCSS();
        bool ret = _embedded_data_RX.addData(data, lcss);
        if(ret)
        {
            processEmbeddedData();
        }
    }
    if((_receiver_state == RECEIVER_STATE::STATE_AUDIO) || (_receiver_state == RECEIVER_STATE::STATE_LATE_ENTRY))
    {
        if((_settings->dmr_mode != DMR_MODE::DMR_MODE_DMO)
                && (_transmitter_state != TRANSMITTER_STATE::STATE_IDLE))
            return true;
        unsigned char *codec_data = new unsigned char[27];
        if(frame.getVoice(codec_data))
            emit digitalAudio(codec_data, 27);
        else
        {
            delete[] codec_data;
            return false;
        }
    }
    return true;
}

bool DMRControl::processCSBK(const DMRFrame &frame)
{
    if(!processColorCode(frame))
        return false;
    if(!processTimeslot(frame))
        return false;
    uint8_t data_type = frame.getDataType();
    unsigned char frame_data[FRAME_LENGTH_BYTES];
    CDMRCSBK csbk;
    frame.getData(frame_data);
    switch(data_type)
    {
        case DT_MBC_HEADER:
        {
            csbk.setDataType(DT_MBC_HEADER);
            csbk.put(frame_data);
            break;
        }
        case DT_MBC_CONTINUATION:
        {
            csbk.setDataType(DT_MBC_CONTINUATION);
            csbk.put(frame_data);
            break;
        }
        default:
        {
            csbk.put(frame_data);
            break;
        }
    }
    return true;
}

bool DMRControl::processVoiceHeader(const DMRFrame &frame)
{
    if(!processColorCode(frame))
        return false;
    if(!processTimeslot(frame))
        return false;
    _color_code_RX = frame.getColorCode();
    if((_receiver_state == RECEIVER_STATE::STATE_IDLE)
            && (_transmitter_state == TRANSMITTER_STATE::STATE_IDLE))
        emit endBeep();
    _srcId_RX = frame.getSrcId();
    _dstId_RX = frame.getDstId();
    _FLCO_RX = frame.getFLCO();
    uint8_t fid = frame.getFID();
    _receiver_state = RECEIVER_STATE::STATE_AUDIO;
    QString mode = (_settings->dmr_mode == DMR_MODE::DMR_MODE_DMO) ? "DMO" : "Repeater";
    QString codec2 = (fid == 0xC2) ? "(Codec2)" : "";
    QString group = (_FLCO_RX == FLCO_GROUP) ? "(Group call)" : "(Private call)";
    QString tg = (_FLCO_RX == FLCO_GROUP) ? "TG" : "";
    QString rx_info = QString("\n\n<br/>DMR %1 <font color=\"#ffaaff\">Voice Start></font> from"
                              " <font color=\"#ffff00\"><b>%2</b></font> to"
                              " <font color=\"#00ff00\"><b>%3 %4</font></b>"
                              " <font color=\"#55aaff\">%5</font> %6<br/>").
            arg(mode).
            arg(_srcId_RX).
            arg(tg).
            arg(_dstId_RX).
            arg(group).
            arg(codec2);
    emit headerReceived(rx_info, true);
    return true;
}

bool DMRControl::processTerminator(const DMRFrame &frame)
{
    if((frame.getDstId() == 0) && (frame.getSrcId() == 0))
        return true; // these are trunking generated terminators
    if(!processColorCode(frame))
        return false;
    if(!processTimeslot(frame))
        return false;
    uint8_t fid = frame.getFID();
    QString mode = (_settings->dmr_mode == DMR_MODE::DMR_MODE_DMO) ? "DMO" : "Repeater";
    QString group = (_FLCO_RX == FLCO_GROUP) ? "(Group call)" : "(Private call)";
    QString codec2 = (fid == 0xC2) ? "(Codec2)" : "";
    QString tg = (_FLCO_RX == FLCO_GROUP) ? "TG" : "";
    QString rx_info = QString("\n\n<br/>DMR %1 <font color=\"#ff557f\">Voice End</font> from"
                              " <font color=\"#ffff00\"><b>%2</b></font> to"
                              " <font color=\"#00ff00\"><b>%3 %4</font></b>"
                              " <font color=\"#55aaff\">%5</font> %6<br/>").
            arg(mode).
            arg(_srcId_RX).
            arg(tg).
            arg(_dstId_RX).
            arg(group).
            arg(codec2);
    if(_receiver_state != RECEIVER_STATE::STATE_IDLE)
    {
        emit terminatorReceived(rx_info, true);
        emit endBeep();
    }
    _srcId_RX = 0;
    _dstId_RX = 0;
    _FLCO_RX = 0;
    _receiver_state = RECEIVER_STATE::STATE_IDLE;
    _talker_alias_received = false;
    _color_code_RX = 0;
    _timeslot_RX = 0;
    return true;
}

bool DMRControl::processDataHeader(const DMRFrame &frame)
{
    if(!processColorCode(frame))
        return false;
    if(!processTimeslot(frame))
        return false;
    _color_code_RX = frame.getColorCode();
    _srcId_RX = frame.getSrcId();
    _dstId_RX = frame.getDstId();
    _FLCO_RX = frame.getFLCO();
    _receiver_state = RECEIVER_STATE::STATE_DATA;
    QString mode = (_settings->dmr_mode == DMR_MODE::DMR_MODE_DMO) ? "DMO" : "Repeater";
    QString group = (_FLCO_RX == FLCO_GROUP) ? "(Group call)" : "(Private call)";
    QString tg = (_FLCO_RX == FLCO_GROUP) ? "TG" : "";
    QString rx_info = QString("\n\n<br/>DMR %1 Data Start from"
                              " <font color=\"#ffff00\"><b>%2</b></font> to"
                              " <font color=\"#00ff00\"><b>%3 %4</font></b>"
                              " <font color=\"#55aaff\">%5</font><br/>").
            arg(mode).
            arg(_srcId_RX).
            arg(tg).
            arg(_dstId_RX).
            arg(group);
    emit headerReceived(rx_info, true);
    return true;
}

bool DMRControl::processDataBlock(const DMRFrame &frame)
{
    if(!processColorCode(frame))
        return false;
    if(!processTimeslot(frame))
        return false;
    if(_receiver_state != RECEIVER_STATE::STATE_DATA)
        return false;
    return true;
}

bool DMRControl::processColorCode(const DMRFrame &frame)
{
    // FIXME: promiscuous breaks in funny ways
    uint8_t color_code = frame.getColorCode();
    if((color_code != _settings->dmr_color_code) && !_settings->dmr_promiscuous_mode)
    {
        if((frame.getDataType() != DT_VOICE)
                || (frame.getDataType() != DT_VOICE_SYNC)
                || (frame.getDataType() != DT_RATE_12_DATA)
                || (frame.getDataType() != DT_RATE_34_DATA)
                || (frame.getDataType() != DT_RATE_1_DATA))
        {
            return false;
        }
    }
    if(_settings->dmr_promiscuous_mode)
    {
        if((color_code != _color_code_RX) && (_color_code_RX != 0))
        {
            return false;
        }
        if(_color_code_RX == 0)
        {
            _color_code_RX = color_code;
        }
    }
    return true;
}

bool DMRControl::processTimeslot(const DMRFrame &frame)
{
    // FIXME: promiscuous breaks in funny ways
    uint8_t timeslot = frame.getSlotNo();
    if(_settings->dmr_mode == DMR_MODE::DMR_MODE_DMO)
        return true;
    if((timeslot != _settings->dmr_timeslot) && !_settings->dmr_promiscuous_mode)
        return false;
    if(_settings->dmr_promiscuous_mode)
    {
        if((timeslot != _timeslot_RX) && (_timeslot_RX != 0))
            return false;
        if(_timeslot_RX == 0)
        {
            _timeslot_RX = timeslot;
        }
    }
    return true;
}

bool DMRControl::processEmbeddedData()
{
    FLCO flco = _embedded_data_RX.getFLCO();
    unsigned char raw_data[9U];
    _embedded_data_RX.getRawData(raw_data);

    switch (flco) {
    case FLCO_GROUP:
    case FLCO_USER_USER:
    {
        if(_embedded_data_RX.isValid())
        {
            CDMRLC *lc = _embedded_data_RX.getLC();
            _srcId_RX = lc->getSrcId();
            _dstId_RX = lc->getDstId();
            _FLCO_RX = lc->getFLCO();
            if(_receiver_state == RECEIVER_STATE::STATE_IDLE)
            {
                _receiver_state = RECEIVER_STATE::STATE_LATE_ENTRY;
            }
            delete lc;
        }
    }
        break;
    case FLCO_GPS_INFO:
    {
        float longitude, latitude = 0.0f;
        std::string error;
        CUtils::extractGPSPosition(raw_data, error, longitude, latitude);
        setGPSInfo(longitude, latitude, error);
    }
        break;

    case FLCO_TALKER_ALIAS_HEADER:
    {
        if(!_talker_alias_received)
        {
            _ta_df = (raw_data[2] >> 6) & 0x03;
            _ta_dl = (raw_data[2] >> 1) & 0x1F;
            _ta_data.clear();
            if(_ta_df == 0)
            {
                // for 7 bit TA the MSB is last bit of byte 3
                _ta_data.append(raw_data[2] & 0x01);
            }
            for(int i=3;i<9;i++)
            {
                _ta_data.append(raw_data[i]);
            }
            processTalkerAlias();
        }
    }
        break;

    case FLCO_TALKER_ALIAS_BLOCK1:
    {
        if(!_talker_alias_received && (_ta_dl > 0))
        {
            for(int i=2;i<9;i++)
            {
                _ta_data.append(raw_data[i]);
            }
            processTalkerAlias();
        }
    }
        break;

    case FLCO_TALKER_ALIAS_BLOCK2:
    {
        if(!_talker_alias_received && (_ta_dl > 0))
        {
            for(int i=2;i<9;i++)
            {
                _ta_data.append(raw_data[i]);
            }
            processTalkerAlias();
        }
    }
        break;

    case FLCO_TALKER_ALIAS_BLOCK3:
    {
        if(!_talker_alias_received && (_ta_dl > 0))
        {
            for(int i=2;i<9;i++)
            {
                _ta_data.append(raw_data[i]);
            }
            processTalkerAlias();
        }
    }
        break;

    default:
        _logger->log(Logger::LogLevelDebug, QString("Unknown Embedded Data %1")
                     .arg(QString::fromLocal8Bit((const char*)raw_data, 9)));
        break;
    }
    return true;
}

void DMRControl::setGPSInfo(float longitude, float latitude, std::string error)
{

    QString gps_info = QString("Longitude: %1, Latitude: %2, Error: %3")
            .arg(longitude)
            .arg(latitude)
            .arg(QString::fromStdString(error));
    //_logger->log(Logger::LogLevelDebug, QString("GPS Info received from %1 to %2: %3")
    //             .arg(QString::number(getSource())).arg(QString::number(getDestination())).arg(_gps_info));
    emit gpsInfo(gps_info, true);
}


void DMRControl::processTalkerAlias()
{
    unsigned int size = _ta_data.size();
    if(size < 1)
        return;
    unsigned int bit7_size = 8 * size / 7;
    if(((_ta_df == 1 || _ta_df == 2) && (size >= _ta_dl)) ||
            ((_ta_df == 3) && (size >= _ta_dl*2)) ||
            ((_ta_df == 0) && (bit7_size >= _ta_dl)))
    {
        if(_ta_df == 1 || _ta_df == 2)
        {
            QString txt = QString::fromUtf8(_ta_data);
            QString talker_alias = QString("\n<br/>Talker alias: <b>%1</b>\n").arg(txt);
            emit talkerAlias(talker_alias, true);
        }
        else if(_ta_df == 0)
        {
            unsigned char converted[bit7_size];
            DMRUtils::parseISO7bitToISO8bit((unsigned char*)_ta_data.constData(), converted, bit7_size, size);
            QString txt = QString::fromUtf8((const char*)converted + 1, bit7_size - 1).trimmed();
            QString talker_alias = QString("\n<br/>Talker alias: <b>%1</b>\n").arg(txt);
            emit talkerAlias(talker_alias, true);
        }
        else if(_ta_df == 3)
        {
            if(QSysInfo::ByteOrder == QSysInfo::BigEndian)
            {
                QString txt = QString::fromUtf16((char16_t*)_ta_data.constData(), size/2);
                QString talker_alias = QString("\n<br/>Talker alias: <b>%1</b>\n").arg(txt);
                emit talkerAlias(talker_alias, true);
            }
            else
            {
                QString txt;
                DMRUtils::parseUTF16(txt, size, (unsigned char*)_ta_data.data());
                QString talker_alias = QString("\n<br/>Talker alias: <b>%1</b>\n").arg(txt);
                emit talkerAlias(talker_alias, true);
            }
        }
        _talker_alias_received = true;
        _ta_data.clear();
        _ta_dl = 0;
        _ta_df = 0;
    }
}

void DMRControl::addFrames(std::vector<DMRFrame> frames)
{
    for(unsigned int i=0;i<frames.size();i++)
    {
        DMRFrame frame = frames.at(i);
        bool valid = frame.validate();
        if(!valid)
        {
            _logger->log(Logger::LogLevelWarning, QString("DMR frame failed validation."));
            continue;
        }
        bool cach_decoded = frame.cachDecoded();
        if(!cach_decoded && (_settings->dmr_mode != DMR_MODE::DMR_MODE_DMO))
        {
            continue;
        }
        uint8_t timeslot = frame.getSlotNo();
        if(((timeslot != _settings->dmr_timeslot) && !_settings->dmr_promiscuous_mode) &&
                (_settings->dmr_mode != DMR_MODE::DMR_MODE_DMO))
            continue;
        uint8_t data_type= frame.getDataType();

        bool process_result = true;

        if((data_type == DT_CSBK) || (data_type == DT_MBC_HEADER) || (data_type == DT_MBC_CONTINUATION))
            process_result = processCSBK(frame);
        else if(data_type == DT_DATA_HEADER)
            process_result = processDataHeader(frame);
        else if(data_type == DT_VOICE_LC_HEADER)
            process_result = processVoiceHeader(frame);
        else if(data_type == DT_TERMINATOR_WITH_LC)
            process_result = processTerminator(frame);
        else if((data_type == DT_VOICE_SYNC) || (data_type == DT_VOICE))
        {
            // FIXME: unrealiable
            frame.setColorCode(_color_code_RX);
            process_result = processAudio(frame);
        }
    }
    frames.clear();
}
