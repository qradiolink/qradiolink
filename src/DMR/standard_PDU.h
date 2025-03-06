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

#ifndef STANDARD_PDU_H
#define STANDARD_PDU_H


enum StandardAddreses
{
    ALLMSI = 0xFFFED4,
    REGI = 0xFFFEC6,
    TSI = 0xFFFECA,
    ALLMSIDL = 0xFFFFFD,
    ALLMSID = 0xFFFFFF,
    SDMI = 0xFFFEC5,
    TATTSI = 0xFFFED7,
    DGNAI = 0xFFFED6,
    DIVERTI = 0xFFFEC9,
    MSI = 0xFFFEC7,
    GPI = 0xFFFECE,
    AUTHI = 0xFFFECD,
    SUPLI = 0xFFFEC4,
    DISPATI = 0xFFFECB,
    LINEI = 0xFFFEC2,
    IPI = 0xFFFEC3,
    HDATA_GW = 0xFFFD02,
};

enum ServiceKind
{
    IndivVoiceCall = 0,
    GroupVoiceCall = 1,
    IndivPacketDataCall = 2,
    GroupPacketDataCall = 3,
    IndivUDTDataCall = 4,
    GroupUDTDataCall = 5,
    UDTDataPolling = 6,
    StatusTransport = 7,
    CallDiversion = 8,
    CallAnswer = 9,
    FullDuplexVoiceCall = 10,
    FullDuplexDataCall = 11,
    SupplementaryServ = 13,
    RegiAuthMSCheck = 14,
    CancelCall = 15
};

enum PollFMT
{
    PollBinary = 0,
    PollAddress = 1,
    Poll4BCD = 2,
    PollISO7 = 3,
    PollISO8 = 4,
    PollNMEA = 5,
    PollIP = 6,
    PollUTF16 = 7,
    PollStatus = 10,
};

enum UDTFormat
{
    FmtBinary = 0,
    FmtAddress = 1,
    Fmt4BCD = 2,
    FmtISO7 = 3,
    FmtISO8 = 4,
    FmtNMEA = 5,
    FmtIP = 6,
    FmtUTF16 = 7,
    FmtMixed = 10,
    FmtLIP = 11,
};


#endif // STANDARD_PDU_H
