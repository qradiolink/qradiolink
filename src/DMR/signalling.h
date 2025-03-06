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

#ifndef SIGNALLING_H
#define SIGNALLING_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QDateTime>
#include "src/logicalchannel.h"
#include "src/utils.h"
#include "src/standard_PDU.h"
#include "src/settings.h"
#include "src/crc32.h"
#include "MMDVM/DMRCSBK.h"
#include "MMDVM/DMRData.h"
#include "MMDVM/DMRDataHeader.h"
#include "MMDVM/DMRDefines.h"

class Signalling
{

public:
    explicit Signalling(Settings *settings);
    void getUABPadNibble(unsigned int msg_size, unsigned int &UAB, unsigned int &pad_nibble);
    void createLateEntryAnnouncement(LogicalChannel *logical_channel, CDMRCSBK &csbk);
    bool createAbsoluteParameters(CDMRCSBK &csbk1, CDMRCSBK &csbk2,
                                                   LogicalChannel *&logical_channel);
    void createRegistrationRequest(CDMRCSBK &csbk);
    void createLogicalPhysicalChannelsAnnouncement(CDMRCSBK &csbk1, CDMRCSBK &csbk_cont, QMap<QString, uint64_t> channel);
    void createAdjacentSiteAnnouncement(CDMRCSBK &csbk, QMap<QString, uint64_t> site);
    void createLocalTimeAnnouncement(CDMRCSBK &csbk, QDateTime date_time);
    void createPresenceCheckAhoy(CDMRCSBK &csbk, unsigned int target_id, bool group);
    void createAuthCheckAhoy(CDMRCSBK &csbk, unsigned int target_id, unsigned int challenge, unsigned char options=0x00);
    void createReplyMessageAccepted(CDMRCSBK &csbk, unsigned int dstId, unsigned int srcId=StandardAddreses::SDMI, bool from_ts=true);
    void createReplyRegistrationAccepted(CDMRCSBK &csbk, unsigned int dstId);
    void createReplyRegistrationRefused(CDMRCSBK &csbk, unsigned int dstId);
    void createReplyRegistrationDenied(CDMRCSBK &csbk, unsigned int dstId);
    void createReplyDeregistrationAccepted(CDMRCSBK &csbk, unsigned int dstId);
    void createReplyCallDivertAccepted(CDMRCSBK &csbk, unsigned int dstId);
    void createPrivateVoiceCallRequest(CDMRCSBK &csbk, bool local, unsigned int srcId, unsigned int dstId);
    void createPrivatePacketCallAhoy(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId);
    void createPrivateVoiceGrant(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int srcId, unsigned int dstId);
    void createGroupVoiceGrant(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int srcId, unsigned int dstId);
    void createPrivatePacketDataGrant(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int srcId, unsigned int dstId);
    void createGroupPacketDataGrant(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int srcId, unsigned int dstId);
    void createClearChannelUserInitiated(CDMRCSBK &csbk, LogicalChannel *logical_channel, unsigned int dstId, bool group_call);
    void createChannelIdleDeallocation(CDMRCSBK &csbk, unsigned int call_type);
    void createRequestToUploadTgAttachments(CDMRCSBK &csbk, unsigned int dstId, unsigned int UAB);
    void createReplyCallRejected(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId);
    void createCancelPrivateCallAhoy(CDMRCSBK &csbk, unsigned int dstId);
    void createCallDisconnect(CDMRCSBK &csbk, unsigned int dstId, bool group_call);
    unsigned int createRequestToUploadMessage(CDMRCSBK &csbk, unsigned int dstId);
    unsigned int createRequestToUploadDivertInfo(CDMRCSBK &csbk, unsigned int dstId);
    void createRequestToUploadUDTPolledData(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId, unsigned int format, unsigned int num_blocks);
    void createRequestToSendGroupCallSupplimentaryData(CDMRCSBK &csbk, unsigned int dstId);
    void createRequestToSendPacketExtendedAddressInfo(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId, uint8_t GI, uint8_t uab);
    void createStatusTransportAhoy(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId, bool group);
    void createStatusPollAhoy(CDMRCSBK &csbk, unsigned int srcId, unsigned int dstId, bool group);
    void createReplyWaitForSignalling(CDMRCSBK &csbk, unsigned int dstId);
    void createReplyCallQueued(CDMRCSBK &csbk, unsigned int dstId);
    void createReplyCallDenied(CDMRCSBK &csbk, unsigned int dstId);
    void createReplyNotRegistered(CDMRCSBK &csbk, unsigned int dstId);
    void createReplyUDTCRCError(CDMRCSBK &csbk, unsigned int dstId);
    void createClearChannelAll(CDMRCSBK &csbk, unsigned int call_type);
    CDMRData createUDTMessageHeader(unsigned int srcId, unsigned int dstId, unsigned int blocks, unsigned int pad_nibble, bool group=false);
    CDMRData createUDTDGNAHeader(unsigned int srcId, unsigned int dstId, unsigned int blocks);
    CDMRData createUDTCallDivertHeader(unsigned int srcId, unsigned int dstId, unsigned int blocks, unsigned int sap=0);
    CDMRData createConfirmedMessageResponseHeader(unsigned int srcId, unsigned int dstId, unsigned int seq_no,
                                                  unsigned int blocks, unsigned int sap, bool group);
    CDMRData createConfirmedDataResponsePayload(unsigned int srcId, unsigned int dstId);
    void rewriteUDTHeader(CDMRData &dmr_data, unsigned int dstId);



private:
    Settings *_settings;
    const uint8_t _uab_pad_nibbles_mapping[2][46] =
    {
        {1,1,1,1,1,1,1,1,1,1,
         2,2,2,2,2,2,2,2,2,2,2,2,
         3,3,3,3,3,3,3,3,3,3,3,3,
         4,4,4,4,4,4,4,4,4,4,4,4},
        {18, 16, 14, 12, 10, 8, 6, 4, 2, 0,
         22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0,
         22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0,
         22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0, }
    };



};

#endif // SIGNALLING_H
