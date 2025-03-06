/*
 *   Copyright (C) 2015,2016,2020,2021,2022 by Jonathan Naylor G4KLX
 *   Copyright (C) 2019 by Patrick Maier DK5MP
 *   Copyright (C) 2023 by Adrian Musceac YO8RZZ
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "DMRCSBK.h"
#include "BPTC19696.h"
#include "Utils.h"
#include "CRC.h"

#include <cstdio>
#include <cassert>
#include <string.h>

CDMRCSBK::CDMRCSBK() :
m_data(NULL),
m_CSBKO(CSBKO_NONE),
m_FID(0x00U),
m_GI(false),
m_bsId(0U),
m_srcId(0U),
m_dstId(0U),
m_dataContent(false),
m_CBF(0U),
m_OVCM(false),
m_LB(false),
m_PF(false),
m_dataType(DT_CSBK)
{
	m_data = new unsigned char[12U];
    memset(m_data, 0, 12U);
}

CDMRCSBK::CDMRCSBK(const CDMRCSBK &csbk) :
m_data(NULL),
m_CSBKO(csbk.m_CSBKO),
m_FID(csbk.m_FID),
m_GI(csbk.m_GI),
m_bsId(csbk.m_bsId),
m_srcId(csbk.m_srcId),
m_dstId(csbk.m_dstId),
m_dataContent(csbk.m_dataContent),
m_CBF(csbk.m_CBF),
m_OVCM(csbk.m_OVCM),
m_LB(csbk.m_LB),
m_PF(csbk.m_PF),
m_dataType(csbk.m_dataType)
{
    m_data = new unsigned char[12U];
    ::memcpy(m_data, csbk.m_data, 12U);
}

CDMRCSBK& CDMRCSBK::operator=(const CDMRCSBK& csbk)
{
    if (this != &csbk) {
        ::memcpy(m_data, csbk.m_data, 12U);

        m_CSBKO   = csbk.m_CSBKO;
        m_FID     = csbk.m_FID;
        m_GI    = csbk.m_GI;
        m_bsId        = csbk.m_bsId;
        m_srcId    = csbk.m_srcId;
        m_dstId    = csbk.m_dstId;
        m_dataContent      = csbk.m_dataContent;
        m_CBF     = csbk.m_CBF;
        m_OVCM = csbk.m_OVCM;
        m_LB    = csbk.m_LB;
        m_PF    = csbk.m_PF;
        m_dataType = csbk.m_dataType;
    }

    return *this;
}

CDMRCSBK::~CDMRCSBK()
{
	delete[] m_data;
}

bool CDMRCSBK::put(const unsigned char* bytes)
{
	assert(bytes != NULL);

	CBPTC19696 bptc;
	bptc.decode(bytes, m_data);

	m_data[10U] ^= CSBK_CRC_MASK[0U];
	m_data[11U] ^= CSBK_CRC_MASK[1U];

	bool valid = CCRC::checkCCITT162(m_data, 12U);
	if (!valid)
		return false;
    m_dataType = DT_CSBK;
	// Restore the checksum
	m_data[10U] ^= CSBK_CRC_MASK[0U];
	m_data[11U] ^= CSBK_CRC_MASK[1U];

	m_CSBKO = CSBKO(m_data[0U] & 0x3FU);
	m_FID   = m_data[1U];

	switch (m_CSBKO) {
	case CSBKO_BSDWNACT:
		m_GI    = false;
		m_bsId  = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U]; 
		m_dataContent = false;
		m_CBF   = 0U;
		CUtils::dump(1U, "Downlink Activate CSBK", m_data, 12U);
		break;

	case CSBKO_UUVREQ:
		m_GI    = false;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
		m_CBF   = 0U;
		m_OVCM  = (m_data[2U] & 0x04U) == 0x04U;
		CUtils::dump(1U, "Unit to Unit Service Request CSBK", m_data, 12U);
		break;

	case CSBKO_UUANSRSP:
		m_GI    = false;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
		m_CBF   = 0U;
		m_OVCM  = (m_data[2U] & 0x04U) == 0x04U;
		CUtils::dump(1U, "Unit to Unit Service Answer Response CSBK", m_data, 12U);
		break;

	case CSBKO_PRECCSBK:
		m_GI    = (m_data[2U] & 0x40U) == 0x40U;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = (m_data[2U] & 0x80U) == 0x80U;
		m_CBF   = m_data[3U];
		CUtils::dump(1U, "Preamble CSBK", m_data, 12U);
		break;

	case CSBKO_NACKRSP:
		m_GI    = false;
		m_srcId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_dstId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
		m_CBF   = 0U;
		CUtils::dump(1U, "Negative Acknowledge Response CSBK", m_data, 12U);
		break;

	case CSBKO_RAND:
		m_GI    = false;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
        m_service_kind = m_data[3U] & 0xFU;
        m_service_options = m_data[2U] >> 1;
        m_CBF   = m_data[3U];
		CUtils::dump(1U, "Call Alert CSBK", m_data, 12U);
		break;

    case CSBKO_MAINT:
        m_GI    = false;
        m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
        m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
        m_dataContent = false;
        m_service_kind = m_data[3U] & 0xFU;
        m_CBF   = m_data[3U];
        CUtils::dump(1U, "Call Alert CSBK", m_data, 12U);
        break;

    case CSBKO_ACKD:
		m_GI    = false;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
        m_CBF   = m_data[3U];
		CUtils::dump(1U, "Call Alert Ack CSBK", m_data, 12U);
		break;

    case CSBKO_ACKU:
        m_GI    = false;
        m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
        m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
        m_dataContent = false;
        m_CBF   = m_data[3U];
        CUtils::dump(1U, "Call Alert Ack CSBK", m_data, 12U);
        break;

	case CSBKO_RADIO_CHECK:
		m_GI    = false;
		if (m_data[3U] == 0x80) {
			m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
			m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
			CUtils::dump(1U, "Radio Check Req CSBK", m_data, 12U);
		} else {
			m_srcId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
			m_dstId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
			CUtils::dump(1U, "Radio Check Ack CSBK", m_data, 12U);
		}
		m_dataContent = false;
		m_CBF   = 0U;
		break;

	default:
        m_GI    = false;
        m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
        m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
        m_dataContent = false;
        m_service_kind = m_data[3U] & 0xFU;
        m_service_options = m_data[2U] >> 1;
        m_CBF   = m_data[3U];
        break;
	}

	return true;
}

void CDMRCSBK::get(unsigned char* bytes) const
{
	assert(bytes != NULL);
    if(m_dataType == DT_CSBK)
    {

        m_data[10U] ^= CSBK_CRC_MASK[0U];
        m_data[11U] ^= CSBK_CRC_MASK[1U];

        CCRC::addCCITT162(m_data, 12U);

        m_data[10U] ^= CSBK_CRC_MASK[0U];
        m_data[11U] ^= CSBK_CRC_MASK[1U];
    }
    else if(m_dataType == DT_MBC_HEADER)
    {
        m_data[10U] ^= MBC_CRC_MASK[0U];
        m_data[11U] ^= MBC_CRC_MASK[1U];

        CCRC::addCCITT162(m_data, 12U);

        m_data[10U] ^= MBC_CRC_MASK[0U];
        m_data[11U] ^= MBC_CRC_MASK[1U];
    }
    else if(m_dataType == DT_MBC_CONTINUATION)
    {
        CCRC::addCCITT162(m_data, 12U);
    }

	CBPTC19696 bptc;
	bptc.encode(m_data, bytes);
}

CSBKO CDMRCSBK::getCSBKO() const
{
	return m_CSBKO;
}

unsigned char CDMRCSBK::getFID() const
{
	return m_FID;
}

bool CDMRCSBK::getOVCM() const
{
	return m_OVCM;
}

void CDMRCSBK::setOVCM(bool ovcm)
{
	if (m_CSBKO == CSBKO_UUVREQ || m_CSBKO == CSBKO_UUANSRSP) {
		m_OVCM = ovcm;

		if (ovcm)
			m_data[2U] |= 0x04U;
		else
			m_data[2U] &= 0xFBU;
	}
}

bool CDMRCSBK::getGI() const
{
	return m_GI;
}

unsigned int CDMRCSBK::getBSId() const
{
	return m_bsId;
}

unsigned int CDMRCSBK::getSrcId() const
{
	return m_srcId;
}

unsigned int CDMRCSBK::getDstId() const
{
	return m_dstId;
}

bool CDMRCSBK::getDataContent() const
{
	return m_dataContent;
}

unsigned char CDMRCSBK::getCBF() const
{
	return m_CBF;
}

void CDMRCSBK::setCBF(unsigned char cbf)
{
	m_CBF = m_data[3U] = cbf;
}

unsigned int CDMRCSBK::getServiceKind() const
{
    return m_service_kind;
}

unsigned int CDMRCSBK::getServiceOptions() const
{
    return m_service_options;
}

void CDMRCSBK::setCSBKO(unsigned char csbko, bool LB, bool PF)
{
    m_data[0] = csbko & 0x3F;
    m_CSBKO = CSBKO(csbko);
    if(LB)
    {
        m_data[0] |= 1 << 7;
        m_LB = true;
    }
    else
    {
        m_data[0] &= 0x7F;;
        m_LB = false;
    }
    if(PF)
    {
        m_data[0] |= 1 << 6;
        m_PF = true;
    }
    else
    {
        m_data[0] &= 0xBF;
        m_PF = false;
    }
}

void CDMRCSBK::setLB(bool LB)
{
    if(LB)
        m_data[0] |= 1 << 7;
    else
        m_data[0] &= 0x7F;
    m_LB = LB;
}

bool CDMRCSBK::getLB()
{
    return m_LB;
}

void CDMRCSBK::setPF(bool PF)
{
    if(PF)
        m_data[0] |= 1 << 6;
    else
        m_data[0] &= 0xBF;
    m_PF = PF;
}

bool CDMRCSBK::getPF()
{
    return m_PF;
}

void CDMRCSBK::setFID(unsigned char FID)
{
    m_data[1] = m_FID = FID;
}

void CDMRCSBK::setData1(unsigned char data1)
{
    m_data[2U] = data1;
}

unsigned char CDMRCSBK::getData1()
{
    return m_data[2U];
}

void CDMRCSBK::setDstId(unsigned int dstId)
{
    m_dstId = dstId;
    m_data[4U] = m_dstId >> 16;
    m_data[5U] = (m_dstId >> 8) & 0xFF ;
    m_data[6U] = m_dstId & 0xFF;
}

void CDMRCSBK::setSrcId(unsigned int srcId)
{
    m_srcId = srcId;
    m_data[7U] = m_srcId >> 16;
    m_data[8U] = (m_srcId >> 8) & 0xFF ;
    m_data[9U] = m_srcId & 0xFF;
}

bool CDMRCSBK::getProxyFlag()
{
    return (bool)(m_data[2] & 0x01);
}

unsigned int CDMRCSBK::getPriority()
{
    return (unsigned int)((m_data[2] >> 1) & 0x03);
}

bool CDMRCSBK::getBroadcast()
{
    return (bool)((m_data[2] >> 4) & 0x01);
}

bool CDMRCSBK::getSuplimentaryData()
{
    return (bool)((m_data[2] >> 5) & 0x01);
}

void CDMRCSBK::setDataType(unsigned char dataType)
{
    m_dataType = dataType;
}

unsigned char CDMRCSBK::getDataType()
{
    return m_dataType;
}





