// Written by Adrian Musceac YO8RZZ , started October 2023.
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

#include "signalling.h"

Signalling::Signalling(Settings *settings)
{
    _settings = settings;
}

void Signalling::getUABPadNibble(unsigned int msg_size, unsigned int &UAB, unsigned int &pad_nibble)
{
    if(msg_size > 46)
        return;
    UAB = _uab_pad_nibbles_mapping[0][msg_size - 1];
    pad_nibble = _uab_pad_nibbles_mapping[1][msg_size - 1];
}

void Signalling::rewriteUDTHeader(CDMRData &dmr_data, unsigned int dstId)
{
    if(dmr_data.getDataType() == DT_DATA_HEADER)
    {
        unsigned char data[DMR_FRAME_LENGTH_BYTES];
        dmr_data.getData(data);
        CDMRDataHeader header;
        header.put(data);
        if(header.getUDT())
        {
            header.setDstId(dstId);
            header.construct();
            header.get(data);
            CDMRSlotType slotType;
            slotType.setColorCode(1);
            slotType.setDataType(DT_DATA_HEADER);
            slotType.getData(data);
            CSync::addDMRDataSync(data, true);
            dmr_data.setData(data);
        }
    }
}

CDMRData Signalling::createUDTMessageHeader(unsigned int srcId, unsigned int dstId,
                                     unsigned int blocks, unsigned int pad_nibble, bool group)
{
    CDMRDataHeader header;
    header.setA(false);
    header.setGI(group);
    header.setRSVD(0x00);
    header.setFormat(0x00);
    header.setSAP(0x00);
    header.setUDTFormat(0x04); // Only ISO8 supported;
    header.setDstId(dstId);
    header.setSrcId(srcId);
    header.setSF(false);
    header.setPF(false);
    header.setBlocks(blocks);
    header.setPadNibble(pad_nibble);
    header.setOpcode(UDTOpcode::C_UDTHD);
    header.construct();
    unsigned char header_data[DMR_FRAME_LENGTH_BYTES];
    header.get(header_data);
    CDMRSlotType slotType;
    slotType.setColorCode(1);
    slotType.setDataType(DT_DATA_HEADER);
    slotType.getData(header_data);
    CSync::addDMRDataSync(header_data, true);
    CDMRData dmr_data_header;
    dmr_data_header.setSeqNo(0);
    dmr_data_header.setN(0);
    dmr_data_header.setDataType(DT_DATA_HEADER);
    dmr_data_header.setDstId(header.getDstId());
    dmr_data_header.setSrcId(header.getSrcId());
    dmr_data_header.setData(header_data);

    return dmr_data_header;
}

CDMRData Signalling::createUDTDGNAHeader(unsigned int srcId, unsigned int dstId,
                                     unsigned int blocks)
{
    CDMRDataHeader header;
    header.setA(true);
    header.setGI(false);
    header.setRSVD(0x00);
    header.setFormat(0x00);
    header.setSAP(0x00);
    header.setUDTFormat(0x01);
    header.setDstId(dstId);
    header.setSrcId(srcId);
    header.setSF(false);
    header.setPF(false);
    header.setBlocks(blocks);
    header.setPadNibble(0);
    header.setOpcode(UDTOpcode::C_DGNAHD);
    header.construct();
    unsigned char header_data[DMR_FRAME_LENGTH_BYTES];
    header.get(header_data);
    CDMRSlotType slotType;
    slotType.setColorCode(1);
    slotType.setDataType(DT_DATA_HEADER);
    slotType.getData(header_data);
    CSync::addDMRDataSync(header_data, true);
    CDMRData dmr_data_header;
    dmr_data_header.setSeqNo(0);
    dmr_data_header.setN(0);
    dmr_data_header.setDataType(DT_DATA_HEADER);
    dmr_data_header.setDstId(header.getDstId());
    dmr_data_header.setSrcId(header.getSrcId());
    dmr_data_header.setData(header_data);

    return dmr_data_header;
}

CDMRData Signalling::createUDTCallDivertHeader(unsigned int srcId, unsigned int dstId,
                                     unsigned int blocks, unsigned int sap)
{
    CDMRDataHeader header;
    header.setA(false);
    header.setGI(false);
    header.setRSVD(0x00);
    header.setFormat(0x00);
    header.setSAP((unsigned char)sap);
    header.setUDTFormat(0x01);
    header.setDstId(dstId);
    header.setSrcId(srcId);
    header.setSF(true);
    header.setPF(false);
    header.setBlocks(blocks);
    header.setPadNibble(0);
    header.setOpcode(UDTOpcode::C_UDTHD);
    header.construct();
    unsigned char header_data[DMR_FRAME_LENGTH_BYTES];
    header.get(header_data);
    CDMRSlotType slotType;
    slotType.setColorCode(1);
    slotType.setDataType(DT_DATA_HEADER);
    slotType.getData(header_data);
    CSync::addDMRDataSync(header_data, true);
    CDMRData dmr_data_header;
    dmr_data_header.setSeqNo(0);
    dmr_data_header.setN(0);
    dmr_data_header.setDataType(DT_DATA_HEADER);
    dmr_data_header.setDstId(header.getDstId());
    dmr_data_header.setSrcId(header.getSrcId());
    dmr_data_header.setData(header_data);

    return dmr_data_header;
}

CDMRData Signalling::createConfirmedMessageResponseHeader(unsigned int srcId, unsigned int dstId, unsigned int seq_no,
                                     unsigned int blocks, unsigned int sap, bool group)
{
    CDMRDataHeader header;
    unsigned char data[10];
    memset(data, 0U, 10U);
    data[0] |= DPF_RESPONSE;
    data[1] |= (sap & 0x0F) << 4;
    data[2] = dstId >> 16;
    data[3] = (dstId >> 8) & 0xFF;
    data[4] = (dstId & 0xFF);
    data[5] = srcId >> 16;
    data[6] = (srcId >> 8) & 0xFF;
    data[7] = (srcId & 0xFF);
    data[8] |= blocks;
    data[9] |= 0 << 6; // TODO: class
    data[9] |= 1 << 3; // TODO: type
    data[9] |= 0;      // TODO: status
    header.setData(data);
    unsigned char header_data[DMR_FRAME_LENGTH_BYTES];
    header.get(header_data);
    CDMRSlotType slotType;
    slotType.setColorCode(1);
    slotType.setDataType(DT_DATA_HEADER);
    slotType.getData(header_data);
    CSync::addDMRDataSync(header_data, true);
    CDMRData dmr_data_header;
    dmr_data_header.setSeqNo(0);
    dmr_data_header.setN(0);
    dmr_data_header.setDataType(DT_DATA_HEADER);
    dmr_data_header.setDstId(header.getDstId());
    dmr_data_header.setSrcId(header.getSrcId());
    dmr_data_header.setFLCO((group ? FLCO_GROUP : FLCO_USER_USER));
    dmr_data_header.setData(header_data);
    return dmr_data_header;
}

CDMRData Signalling::createConfirmedDataResponsePayload(unsigned int srcId, unsigned int dstId)
{
    // TODO: set retry block no. bits
    unsigned char dblock[12U];
    unsigned char crc_data[8U];
    memset(dblock, 0xFF, 5U);
    dblock[0U] = 0xFF;
    dblock[2U] = dblock[2U] & 0x0F;
    dblock[3U] = dblock[3U] & 0x07;
    dblock[4U] = dblock[4U] & 0x03;
    memset(dblock + 5U, 0, 4U);
    for(uint i =0;i < 8U;i=i+2)
    {
        crc_data[i] = dblock[i+1];
        crc_data[i+1] = dblock[i];
    }
    crc_t crc = crc32_init();
    crc = crc32_update(crc, crc_data, 8U);
    crc = crc32_finalize(crc);
    dblock[8U] = (crc >> 24) & 0xFF;
    dblock[9U] = (crc >> 16) & 0xFF;
    dblock[10U] = (crc >> 8) & 0xFF;
    dblock[11U] = crc & 0xFF;
    unsigned char payload_data[DMR_FRAME_LENGTH_BYTES];
    CBPTC19696 bptc;
    bptc.encode(dblock, payload_data);
    CDMRSlotType slotType;
    slotType.putData(payload_data);
    slotType.setColorCode(1);
    slotType.setDataType(DT_RATE_12_DATA);
    slotType.getData(payload_data);
    CSync::addDMRDataSync(payload_data, true);
    CDMRData dmr_data;
    dmr_data.setSeqNo(0);
    dmr_data.setN(0);
    dmr_data.setDataType(DT_RATE_12_DATA);
    dmr_data.setDstId(dstId);
    dmr_data.setSrcId(srcId);
    dmr_data.setFLCO(FLCO_USER_USER);
    dmr_data.setData(payload_data);
    return dmr_data;
}

void Signalling::createLateEntryAnnouncement(LogicalChannel *logical_channel, CDMRCSBK &csbk)
{
    uint8_t emergency_call = 0; // TODO
    uint8_t late_entry = 1; // TODO
    if(logical_channel->getCallType() == CallType::CALL_TYPE_GROUP)
    {
        csbk.setCSBKO(CSBKO_TV_GRANT);
    }
    else if(logical_channel->getCallType() == CallType::CALL_TYPE_MS)
    {
        csbk.setCSBKO(CSBKO_PV_GRANT);
    }
    else if(logical_channel->getCallType() == CallType::CALL_TYPE_INDIV_PACKET)
    {
        csbk.setCSBKO(CSBKO_PD_GRANT);
    }
    else if(logical_channel->getCallType() == CallType::CALL_TYPE_GROUP_PACKET)
    {
        csbk.setCSBKO(CSBKO_TD_GRANT);
    }
    uint16_t phys_chan = logical_channel->getLogicalChannel();
    uint8_t c1 = phys_chan >> 4;
    csbk.setData1(c1);
    uint8_t aligned_timing = 0; //TODO
    uint8_t c2 = (phys_chan << 4) & 0xFF;
    uint8_t data2 = c2;
    data2 |= ((logical_channel->getSlot() - 1) << 3);
    data2 |= late_entry << 2;
    data2 |= emergency_call << 1;
    data2 |= aligned_timing;
    csbk.setCBF(data2);
    if((logical_channel->getCallType() == CallType::CALL_TYPE_GROUP) ||
            (logical_channel->getCallType() == CallType::CALL_TYPE_GROUP_PACKET))
    {
        csbk.setDstId(Utils::convertBase10ToBase11GroupNumber(logical_channel->getDestination()));
    }
    else
    {
        csbk.setDstId(logical_channel->getDestination());
    }
    csbk.setSrcId(logical_channel->getSource());
}

bool Signalling::createAbsoluteParameters(CDMRCSBK &csbk1, CDMRCSBK &csbk2,
                                               LogicalChannel *&logical_channel)
{
    if(!_settings->use_absolute_channel_grants)
        return false;
    if(logical_channel == nullptr)
    {
        return false;
    }
    uint64_t params;
    uint8_t colour_code;
    bool valid = logical_channel->getChannelParams(params, colour_code);
    if(!valid)
    {
        return false;
    }
    // Continuation CSBK
    csbk2.setCSBKO(csbk1.getCSBKO());
    csbk2.setFID((unsigned char)colour_code & 0x0F);
    csbk2.setData1((params >> 56) & 0xFF);
    csbk2.setCBF((params >> 48) & 0xFF);
    csbk2.setDstId((params >> 24) & 0xFFFFFF);
    csbk2.setSrcId(params & 0xFFFFFF);
    csbk2.setDataType(DT_MBC_CONTINUATION);

    // rewrite Header
    csbk1.setLB(false);
    // erase logical channel
    csbk1.setData1(0xFF);
    unsigned char cbf = csbk1.getCBF() | 0xF0;
    csbk1.setCBF(cbf);
    csbk1.setDataType(DT_MBC_HEADER);
    return true;
}

void Signalling::createRegistrationRequest(CDMRCSBK &csbk)
{
    uint8_t announcement_type = 0x04 << 3; // MassReg
    uint8_t reg = 1;
    uint8_t par = 3; // PAR AB
    uint16_t system_id = (uint16_t)_settings->system_identity_code << 2;
    system_id = system_id | par;
    uint16_t bcast_parms1 = 8 << 2; // Reg Window (100 sec) + ALOHA mask
    uint32_t bcast_parms2 = 0;
    csbk.setCSBKO(CSBKO_C_BCAST);
    csbk.setFID(0x00);
    unsigned char data1 = (unsigned char) announcement_type;
    csbk.setData1(data1);
    unsigned char data2 = bcast_parms1;
    csbk.setCBF(data2);
    unsigned int data3 = (reg << 4 ) << 16;
    data3 |= 8 << 16; // random backoff
    data3 |= system_id;
    csbk.setDstId(data3);
    csbk.setSrcId(bcast_parms2);
}

void Signalling::createLogicalPhysicalChannelsAnnouncement(CDMRCSBK &csbk1, CDMRCSBK &csbk_cont, QMap<QString, uint64_t> channel)
{
    uint8_t announcement_type = 0x05 << 3;
    uint32_t reg = 1;
    uint16_t system_id = (uint16_t)_settings->system_identity_code << 2;
    system_id = system_id | 0x03; // AB MSs
    uint32_t bcast_parms2 = (!_settings->use_fixed_channel_plan) ?
                channel.value("logical_channel") :
                (channel.value("tx_freq") - _settings->freq_base) / _settings->freq_separation + 1;;
    csbk1.setCSBKO(CSBKO_C_BCAST, false, false);
    csbk1.setFID(0x00);
    unsigned char data1 = (unsigned char) announcement_type;
    csbk1.setData1(data1);
    csbk1.setCBF(0x00);
    unsigned int data3 = 0;

    data3 |= system_id;
    data3 |= 2 << 16; // backoff
    data3 |= reg << 20;
    csbk1.setDstId(data3);
    csbk1.setSrcId(bcast_parms2);
    csbk1.setDataType(DT_MBC_HEADER);

    csbk_cont.setCSBKO(CSBKO_C_BCAST);
    csbk_cont.setFID(0x00);

    uint64_t lcn = (!_settings->use_fixed_channel_plan) ?
                channel.value("logical_channel") :
                (channel.value("tx_freq") - _settings->freq_base) / _settings->freq_separation + 1;;
    uint64_t tx_value_khz = channel.value("tx_freq") % 1000000 / 125;
    uint64_t rx_value_khz = channel.value("rx_freq") % 1000000 / 125;
    uint64_t tx_value_Mhz = channel.value("tx_freq") / 1000000;
    uint64_t rx_value_Mhz = channel.value("rx_freq") / 1000000;
    uint64_t params = 0;
    params |= rx_value_khz;
    params |= rx_value_Mhz << 13;
    params |= tx_value_khz << 23;
    params |= tx_value_Mhz << 36;
    params |= lcn << 46;
    csbk_cont.setData1((params >> 56) & 0xFF);
    csbk_cont.setCBF((params >> 48) & 0xFF);
    csbk_cont.setDstId((params >> 24) & 0xFFFFFF);
    csbk_cont.setSrcId(params & 0xFFFFFF);
    csbk_cont.setDataType(DT_MBC_CONTINUATION);
}

void Signalling::createAdjacentSiteAnnouncement(CDMRCSBK &csbk, QMap<QString, uint64_t> site)
{
    uint8_t announcement_type = 0x06 << 3;
    uint32_t reg = 1;
    uint16_t system_id = (uint16_t)_settings->system_identity_code << 2;
    system_id = system_id | 0x03; // AB MSs
    uint32_t bcast_parms1 = site.value("system_id");
    uint32_t bcast_parms2 = (!_settings->use_fixed_channel_plan) ?
                site.value("logical_channel") :
                (site.value("tx_freq") - _settings->freq_base) / _settings->freq_separation + 1;;
    uint32_t confirmed_priority = 1;
    uint32_t adjacent_priority = 1; // TODO
    uint32_t active_connection = 3;
    bcast_parms2 |= active_connection << 22;
    bcast_parms2 |= confirmed_priority << 19;
    bcast_parms2 |= adjacent_priority << 16;
    csbk.setCSBKO(CSBKO_C_BCAST);
    csbk.setFID(0x00);
    unsigned char data1 = (unsigned char) announcement_type | ((bcast_parms1 >> 11) & 0x03);
    unsigned char data2 = (unsigned char) ((bcast_parms1 >> 3) & 0xFF);
    csbk.setData1(data1);
    csbk.setCBF(data2);
    unsigned int data3 = ((bcast_parms1 & 0x03) << 21);

    data3 |= system_id;
    data3 |= 2 << 16; // backoff
    data3 |= reg << 20;
    csbk.setDstId(data3);
    csbk.setSrcId(bcast_parms2);
}

void Signalling::createLocalTimeAnnouncement(CDMRCSBK &csbk, QDateTime date_time)
{
    uint32_t reg = 1;
    uint16_t system_id = (uint16_t)_settings->system_identity_code << 2;
    system_id = system_id | 0x03; // AB MSs
    uint8_t announcement_type = 0x03 << 3;
    unsigned int broadcast_parms1 = 0;
    broadcast_parms1 |= ((unsigned int)date_time.date().day() & 0x1F) << 9;
    broadcast_parms1 |= ((unsigned int)date_time.date().month() & 0x0F) << 5;
    broadcast_parms1 |= ((unsigned int)date_time.offsetFromUtc() & 0x0F) << 1;
    broadcast_parms1 |= (uint8_t)(date_time.offsetFromUtc() < 0);
    unsigned int bcast_parms2 = 0;
    bcast_parms2 |= ((unsigned int)date_time.time().hour() & 0x1F) << 19;
    bcast_parms2 |= ((unsigned int)date_time.time().minute() & 0x3F) << 13;
    bcast_parms2 |= ((unsigned int)date_time.time().second() & 0x3F) << 7;
    bcast_parms2 |= ((unsigned int)date_time.date().dayOfWeek() & 0x07) << 4;
    csbk.setCSBKO(CSBKO_C_BCAST);
    csbk.setFID(0x00);
    unsigned int data1 = 0;
    data1 |= announcement_type;
    data1 |= (broadcast_parms1 >> 11) & 0x07;
    csbk.setData1((unsigned char)data1);
    unsigned int data2 = (broadcast_parms1 >> 3) & 0xFF;
    csbk.setCBF((unsigned char)data2);
    unsigned int data3 = 0;
    data3 |= ((broadcast_parms1 << 5) & 0xFF) << 21;
    data3 |= system_id;
    data3 |= 2 << 16; // backoff
    data3 |= reg << 20;
    csbk.setDstId(data3);
    csbk.setSrcId(bcast_parms2);
}

void Signalling::createPresenceCheckAhoy(CDMRCSBK &csbk, unsigned int target_id, bool group)
{
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    csbk.setData1(0x00);
    unsigned char data2 = ServiceKind::RegiAuthMSCheck;
    data2 |= (group) ? (1 << 6) : (0 << 6);
    csbk.setCBF(data2);
    csbk.setDstId(target_id & 0xFFFFFF);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createAuthCheckAhoy(CDMRCSBK &csbk, unsigned int target_id, unsigned int challenge, unsigned char options)
{
    unsigned char data1 = options << 1;
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    csbk.setData1(data1);
    unsigned char data2 = ServiceKind::RegiAuthMSCheck;
    csbk.setCBF(data2);
    csbk.setDstId(target_id & 0xFFFFFF);
    csbk.setSrcId(challenge & 0xFFFFFF);
}

void Signalling::createReplyMessageAccepted(CDMRCSBK &csbk, unsigned int dstId, unsigned int srcId, bool from_ts)
{
    csbk.setCSBKO(CSBKO_ACKD);
    csbk.setFID(0x00);
    unsigned int response_info = 0x00;
    unsigned int reason = 0x60; // reason: message_accepted
    if(!from_ts)
        reason = 0x44;
    unsigned int data1 = (response_info << 1) | reason >> 7;
    unsigned int data2 = (reason << 1);
    csbk.setData1(data1);
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createReplyRegistrationAccepted(CDMRCSBK &csbk, unsigned int dstId)
{
    uint8_t accepted_registrations_mask = 0xFE; // be generous in case the manufacturer isn't
    csbk.setCSBKO(CSBKO_ACKD);
    unsigned char data1 = (unsigned char) accepted_registrations_mask;
    csbk.setData1(data1);
    csbk.setCBF(0xC4);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::REGI);
}

void Signalling::createReplyRegistrationRefused(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_ACKD);
    unsigned int response_info = 0;
    unsigned int reason_code = 0x2A; // reason code 0010 1010 registration refused
    unsigned char data1 = 0x00;
    data1 |= response_info << 1;
    data1 |= (reason_code >> 7) & 0xFF;
    csbk.setData1(data1);
    unsigned char data2 = (reason_code << 1) & 0xFF;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::REGI);
}

void Signalling::createReplyRegistrationDenied(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_ACKD);
    unsigned int response_info = 0;
    unsigned int reason_code = 0x2B; // reason code 0010 1011 registration denied
    unsigned char data1 = 0x00;
    data1 |= response_info << 1;
    data1 |= (reason_code >> 7) & 0xFF;
    csbk.setData1(data1);
    unsigned char data2 = (reason_code << 1) & 0xFF;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::REGI);
}

void Signalling::createReplyCallDivertAccepted(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_ACKD);
    csbk.setData1(0x00); // 0x60
    csbk.setCBF(0xC0);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::DIVERTI);
}

void Signalling::createPrivateVoiceCallRequest(CDMRCSBK &csbk, bool local, unsigned int srcId, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_AHOY);
    unsigned int service_kind_flag = 1; // FOACSU: 1, OACSU: 0
    if(!local)
        service_kind_flag = 0;
    csbk.setFID(0x00);
    csbk.setData1(service_kind_flag);
    csbk.setCBF(0x00);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createPrivatePacketCallAhoy(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_AHOY);
    unsigned int service_kind_flag = 0;
    unsigned int service_kind = 0x02;
    unsigned int service_options = csbk.getServiceOptions();
    unsigned char data1 = (unsigned char)((service_options << 1) | service_kind_flag);
    unsigned char data2 = service_kind;
    csbk.setFID(0x00);
    csbk.setData1(data1);
    csbk.setCBF(data2);
    csbk.setDstId(srcId);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createPrivateVoiceGrant(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int srcId, unsigned int dstId)
{
    uint8_t emergency_call = 0; // TODO
    uint8_t late_entry = 0; // TODO
    csbk.setCSBKO(CSBKO_PV_GRANT);
    csbk.setFID(0x00);
    unsigned int phys_chan = logical_channel->getLogicalChannel();
    unsigned char c1 = (phys_chan >> 4) & 0xFF;
    csbk.setData1(c1);
    unsigned char aligned_timing = 0x00; //TODO
    unsigned char c2 = (phys_chan & 0xFF) << 4;
    unsigned char data2 = c2;
    data2 |= ((logical_channel->getSlot() - 1) << 3) & 0x08;
    data2 |= (late_entry << 2) & 0x04;
    data2 |= (emergency_call << 1) & 0x02;
    data2 |= aligned_timing;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createGroupVoiceGrant(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int srcId, unsigned int dstId)
{
    uint8_t emergency_call = 0; // TODO
    uint8_t late_entry = 0;
    csbk.setCSBKO(CSBKO_TV_GRANT);
    csbk.setFID(0x00);
    unsigned int phys_chan = logical_channel->getLogicalChannel();
    unsigned char c1 = (phys_chan >> 4) & 0xFF;
    csbk.setData1(c1);
    unsigned char aligned_timing = 0x00; //TODO
    unsigned char c2 = (phys_chan & 0xFF) << 4;
    unsigned char data2 = c2;
    data2 |= ((logical_channel->getSlot() - 1) << 3) & 0x08;
    data2 |= (late_entry << 2) & 0x04;
    data2 |= (emergency_call << 1) & 0x02;
    data2 |= aligned_timing;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createPrivatePacketDataGrant(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int srcId, unsigned int dstId)
{
    unsigned int service_options = csbk.getServiceOptions();
    bool SIMI = (bool) (service_options >> 2) & 0x01;
    bool hi_rate = (bool) (service_options >> 3) & 0x01; // TODO
    uint8_t emergency_call = 0; // TODO
    uint8_t rate = hi_rate ? 1 : 0; // TODO
    unsigned char csbko = SIMI ? CSBKO_PD_GRANT_MI : CSBKO_PD_GRANT;
    csbk.setCSBKO(csbko);
    csbk.setFID(0x00);
    unsigned int phys_chan = logical_channel->getLogicalChannel();
    unsigned char c1 = (phys_chan >> 4) & 0xFF;
    csbk.setData1(c1);
    unsigned char aligned_timing = 0x00; // TODO: aligned timing doesn't work with dual slot packet data
    unsigned char c2 = (phys_chan & 0xFF) << 4;
    unsigned char data2 = c2;
    data2 |= ((logical_channel->getSlot() - 1) << 3) & 0x08;
    data2 |= (rate << 2) & 0x04;
    data2 |= (emergency_call << 1) & 0x02;
    data2 |= aligned_timing;
    csbk.setCBF(data2);
    csbk.setDstId(srcId);
    csbk.setSrcId(dstId);
}

void Signalling::createGroupPacketDataGrant(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int srcId, unsigned int dstId)
{
    unsigned int service_options = csbk.getServiceOptions();
    bool SIMI = (bool) (service_options >> 2) & 0x01;
    bool hi_rate = (bool) (service_options >> 3) & 0x01; // TODO
    uint8_t emergency_call = 0; // TODO
    uint8_t rate = hi_rate ? 1 : 0; // TODO
    unsigned char csbko = SIMI ? CSBKO_TD_GRANT_MI : CSBKO_TD_GRANT;
    csbk.setCSBKO(csbko);
    csbk.setFID(0x00);
    unsigned int phys_chan = logical_channel->getLogicalChannel();
    unsigned char c1 = (phys_chan >> 4) & 0xFF;
    csbk.setData1(c1);
    unsigned char aligned_timing = 0x00; // TODO: aligned timing doesn't work with dual slot packet data
    unsigned char c2 = (phys_chan & 0xFF) << 4;
    unsigned char data2 = c2;
    data2 |= ((logical_channel->getSlot() - 1) << 3) & 0x08;
    data2 |= (rate << 2) & 0x04;
    data2 |= (emergency_call << 1) & 0x02;
    data2 |= aligned_timing;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createClearChannelUserInitiated(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int dstId, bool group_call)
{
    csbk.setCSBKO(CSBKO_P_CLEAR);
    uint16_t channel = 0;
    uint8_t c1 = channel >> 4;
    csbk.setData1(c1);
    uint8_t c2 = (channel << 4) & 0xFF;
    c2 |= (uint8_t)group_call;
    csbk.setCBF(c2);
    dstId = logical_channel->getDestination();
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createChannelIdleDeallocation(CDMRCSBK &csbk, unsigned int call_type)
{
    csbk.setCSBKO(CSBKO_P_CLEAR);
    csbk.setFID(0x00);
    uint16_t channel = 0;
    uint8_t c1 = channel >> 4;
    csbk.setData1(c1);
    uint8_t c2 = (channel << 4) & 0xFF;
    c2 |= (uint8_t)call_type;
    csbk.setCBF(c2);
    // All clear
    //csbk.setDstId(_logical_channels.at(channel_id)->getDestination());
    csbk.setDstId(StandardAddreses::ALLMSI);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createRequestToUploadTgAttachments(CDMRCSBK &csbk, unsigned int dstId, unsigned int UAB)
{
    unsigned int data1 = csbk.getServiceOptions() << 1;
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    data1 |= 1;
    unsigned int data2 = (UAB << 4) | csbk.getServiceKind();
    csbk.setData1(data1);
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TATTSI);
}

void Signalling::createReplyCallRejected(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId)
{
    unsigned char recipient_refused = 0x14;
    csbk.setCSBKO(CSBKO_ACKD);
    csbk.setFID(0x00);
    csbk.setData1(0x00);
    csbk.setCBF(recipient_refused << 1);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createCancelPrivateCallAhoy(CDMRCSBK &csbk, unsigned int dstId)
{
    unsigned char cancel_call = 0x0F;
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    csbk.setData1(0x00); // Reponse info (not set) + 1 bit reason code 1010 0000
    csbk.setCBF(cancel_call);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createCallDisconnect(CDMRCSBK &csbk, unsigned int dstId, bool group_call)
{
    csbk.setCSBKO(CSBKO_P_CLEAR);
    uint16_t channel = 0;
    uint8_t c1 = channel >> 4;
    csbk.setData1(c1);
    uint8_t c2 = (channel << 4) & 0xFF;
    c2 |= (uint8_t)group_call;
    csbk.setCBF(c2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TSI);
}

unsigned int Signalling::createRequestToUploadMessage(CDMRCSBK &csbk, unsigned int dstId)
{
    unsigned int number_of_blocks = (csbk.getCBF() >> 4) & 0x03;
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    unsigned int data2 = (number_of_blocks << 4) | csbk.getServiceKind();
    csbk.setData1(0x00);
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::SDMI);
    return number_of_blocks;
}

unsigned int Signalling::createRequestToUploadDivertInfo(CDMRCSBK &csbk, unsigned int dstId)
{
    unsigned int number_of_blocks = (csbk.getCBF() >> 4) & 0x03;
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    unsigned int data2 = (number_of_blocks << 4) | csbk.getServiceKind();
    csbk.setData1(0x00);
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::DIVERTI);
    return number_of_blocks;
}

void Signalling::createRequestToUploadUDTPolledData(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId,
                                                    unsigned int format, unsigned int num_blocks)
{
    unsigned int service_options = format << 1;
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    unsigned int data2 = (num_blocks << 4) | ServiceKind::UDTDataPolling;
    csbk.setData1(service_options);
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createRequestToSendGroupCallSupplimentaryData(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    unsigned char data1 = csbk.getServiceOptions() << 1;
    unsigned int data2 = csbk.getServiceKind();
    csbk.setData1(data1);
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createRequestToSendPacketExtendedAddressInfo(CDMRCSBK &csbk, unsigned int srcId,
                                                              unsigned int dstId, uint8_t GI, uint8_t uab)
{
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    unsigned char data1 = csbk.getServiceOptions() << 1;
    unsigned int data2 = ServiceKind::IndivPacketDataCall;
    data2 |= (uab & 0x03) << 4;
    data2 |= (GI & 0x01) << 6;
    csbk.setData1(data1);
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createStatusTransportAhoy(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId, bool group)
{
    unsigned int status2 = (csbk.getCBF() >> 4) & 0x03;
    status2 |= (group) ? 1 << 2 : 0;
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    unsigned int data2 = (status2 << 4) | ServiceKind::StatusTransport;
    unsigned int data1 = (csbk.getData1() >> 1) << 1;
    csbk.setData1(data1);
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createStatusPollAhoy(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId, bool group)
{
    unsigned int status2 = 0x03;
    status2 |= (group) ? 1 << 2 : 0;
    csbk.setCSBKO(CSBKO_AHOY);
    csbk.setFID(0x00);
    unsigned int data2 = (status2 << 4) | ServiceKind::StatusTransport;
    unsigned int data1 = 0x3E;
    data1 |= (group) ? (1 << 7) : 0;
    csbk.setData1(data1);
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(srcId);
}

void Signalling::createReplyWaitForSignalling(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_ACKD);
    csbk.setFID(0x00);
    csbk.setData1(0x01); // Reponse info (not set) + 1 bit reason code 1010 0000
    csbk.setCBF(0x40);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createReplyCallQueued(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_ACKD);
    unsigned int response_info = 0;
    unsigned int reason_code = 160; // reason code 1010 0000 queued
    unsigned char data1 = 0x00;
    data1 |= response_info << 1;
    data1 |= (reason_code >> 7) & 0xFF;
    csbk.setData1(data1); // Reponse info (not set) + 1 bit reason code
    unsigned char data2 = (reason_code << 1) & 0xFF;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createReplyCallDenied(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_ACKD);
    unsigned int response_info = 0;
    unsigned int reason_code = 39; // reason code 0010 0111 busy
    unsigned char data1 = 0x00;
    data1 |= response_info << 1;
    data1 |= (reason_code >> 7) & 0xFF;
    csbk.setData1(data1);
    unsigned char data2 = (reason_code << 1) & 0xFF;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createReplyNotRegistered(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_ACKD);
    unsigned int response_info = 0;
    unsigned int reason_code = 45;
    unsigned char data1 = 0x00;
    data1 |= response_info << 1;
    data1 |= (reason_code >> 7) & 0xFF;
    csbk.setData1(data1);
    unsigned char data2 = (reason_code << 1) & 0xFF;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createReplyDeregistrationAccepted(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_ACKD);
    unsigned int response_info = 0;
    unsigned int reason_code = 98;
    unsigned char data1 = 0x00;
    data1 |= response_info << 1;
    data1 |= (reason_code >> 7) & 0xFF;
    csbk.setData1(data1);
    unsigned char data2 = (reason_code << 1) & 0xFF;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::REGI);
}

void Signalling::createReplyUDTCRCError(CDMRCSBK &csbk, unsigned int dstId)
{
    csbk.setCSBKO(CSBKO_ACKD);
    unsigned int response_info = 0;
    unsigned int reason_code = 48; // reason code 0010 0111 busy
    unsigned char data1 = 0x00;
    data1 |= response_info << 1;
    data1 |= (reason_code >> 7) & 0xFF;
    csbk.setData1(data1);
    unsigned char data2 = (reason_code << 1) & 0xFF;
    csbk.setCBF(data2);
    csbk.setDstId(dstId);
    csbk.setSrcId(StandardAddreses::TSI);
}

void Signalling::createClearChannelAll(CDMRCSBK &csbk, unsigned int call_type)
{
    csbk.setCSBKO(CSBKO_P_CLEAR);
    csbk.setFID(0x00);
    uint16_t channel = 0;
    uint8_t c1 = channel >> 4;
    csbk.setData1(c1);
    uint8_t c2 = channel << 4;
    c2 |= (uint8_t)call_type;
    csbk.setCBF(c2);
    // All clear
    csbk.setDstId(StandardAddreses::ALLMSI);
    csbk.setSrcId(StandardAddreses::TSI);
}

