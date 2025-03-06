/*
 *   Copyright (C) 2012 by Ian Wraith
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
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

#include "DMRDataHeader.h"
#include "DMRDefines.h"
#include "BPTC19696.h"
#include "RS129.h"
#include "Utils.h"
#include "CRC.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char UDTF_NMEA = 0x05U;

CDMRDataHeader::CDMRDataHeader() :
m_data(NULL),
m_GI(false),
m_A(false),
m_srcId(0U),
m_dstId(0U),
m_blocks(0U),
m_padNibble(0U),
m_F(false),
m_S(false),
m_Ns(0U),
m_UDT(false),
m_UDTFormat(0x00),
m_rsvd(0x00),
m_format(0x00),
m_sap(0x00),
m_opcode(0x00),
m_SF(false),
m_PF(false),
m_DPF(0x00)
{
	m_data = new unsigned char[12U];
}

CDMRDataHeader::~CDMRDataHeader()
{
	delete[] m_data;
}

bool CDMRDataHeader::put(const unsigned char* bytes)
{
	assert(bytes != NULL);

	CBPTC19696 bptc;
	bptc.decode(bytes, m_data);

	m_data[10U] ^= DATA_HEADER_CRC_MASK[0U];
	m_data[11U] ^= DATA_HEADER_CRC_MASK[1U];

	bool valid = CCRC::checkCCITT162(m_data, 12U);
	if (!valid)
		return false;

	// Restore the checksum
	m_data[10U] ^= DATA_HEADER_CRC_MASK[0U];
	m_data[11U] ^= DATA_HEADER_CRC_MASK[1U];

	m_GI = (m_data[0U] & 0x80U) == 0x80U;
	m_A  = (m_data[0U] & 0x40U) == 0x40U;

	unsigned char dpf = m_data[0U] & 0x0FU;
	if (dpf == DPF_PROPRIETARY)
		return true;
    m_DPF = dpf;
	m_dstId = m_data[2U] << 16 | m_data[3U] << 8 | m_data[4U];
	m_srcId = m_data[5U] << 16 | m_data[6U] << 8 | m_data[7U];

	switch (dpf) {
	case DPF_UNCONFIRMED_DATA:
		CUtils::dump(1U, "DMR, Unconfirmed Data Header", m_data, 12U);
		m_F = (m_data[8U] & 0x80U) == 0x80U;
		m_blocks = m_data[8U] & 0x7FU;
        m_padNibble = (((m_data[0U] >> 4) & 0x01) << 4) | (m_data[1U] & 0x0F);
		break;

	case DPF_CONFIRMED_DATA:
		CUtils::dump(1U, "DMR, Confirmed Data Header", m_data, 12U);
		m_F = (m_data[8U] & 0x80U) == 0x80U;
		m_blocks = m_data[8U] & 0x7FU;
		m_S = (m_data[9U] & 0x80U) == 0x80U;
		m_Ns = (m_data[9U] >> 4) & 0x07U;
        m_padNibble = (((m_data[0U] >> 4) & 0x01) << 4) | (m_data[1U] & 0x0F);
        m_sap = (m_data[1U] >> 4) & 0x0FU;
		break;

	case DPF_RESPONSE:
		CUtils::dump(1U, "DMR, Response Data Header", m_data, 12U);
		m_blocks = m_data[8U] & 0x7FU;
		break;

	case DPF_PROPRIETARY:
		CUtils::dump(1U, "DMR, Proprietary Data Header", m_data, 12U);
		break;

	case DPF_DEFINED_RAW:
		CUtils::dump(1U, "DMR, Raw or Status/Precoded Short Data Header", m_data, 12U);
		m_blocks = (m_data[0U] & 0x30U) + (m_data[1U] & 0x0FU);
		m_F = (m_data[8U] & 0x01U) == 0x01U;
		m_S = (m_data[8U] & 0x02U) == 0x02U;
		break;

	case DPF_DEFINED_SHORT:
		CUtils::dump(1U, "DMR, Defined Short Data Header", m_data, 12U);
		m_blocks = (m_data[0U] & 0x30U) + (m_data[1U] & 0x0FU);
		m_F = (m_data[8U] & 0x01U) == 0x01U;
		m_S = (m_data[8U] & 0x02U) == 0x02U;
		break;

	case DPF_UDT:
		CUtils::dump(1U, "DMR, Unified Data Transport Header", m_data, 12U);
		m_blocks = (m_data[8U] & 0x03U) + 1U;
        m_UDTFormat = (m_data[1U] & 0x0F);
        m_opcode = (m_data[9U] & 0x3F);
        m_sap = (m_data[1U] >> 4);
        m_rsvd = (m_data[0U] >> 4) & 0x03;
        m_padNibble = m_data[8U] >> 3;
        m_PF = (m_data[9U] >> 6) & 0x01;
        m_SF = m_data[9U] >> 7;
        m_format = m_data[0U] & 0x0F;
        m_UDT = true;
		break;

	default:
		CUtils::dump("DMR, Unknown Data Header", m_data, 12U);
		break;
	}

	return true;
}

void CDMRDataHeader::get(unsigned char* bytes) const
{
	assert(bytes != NULL);
    if(m_UDT)
    {
        // Table B.1: CSBK/MBC/UDT Opcode List
        // Convert to Unified Data Transport outbound Header
        m_data[9U] &= 0xFE;
    }
    CCRC::addCCITT162(m_data, 12U);
    // Restore the checksum
    m_data[10U] ^= DATA_HEADER_CRC_MASK[0U];
    m_data[11U] ^= DATA_HEADER_CRC_MASK[1U];
	CBPTC19696 bptc;
	bptc.encode(m_data, bytes);
}

void CDMRDataHeader::construct()
{
    ::memset(m_data, 0, 11U);
    m_UDT = true;
    m_data[0] |= ((unsigned int)m_GI) << 7;
    m_data[0] |= ((unsigned int)m_A) << 6;
    m_data[0] |= (m_rsvd & 0x03) << 4;
    m_data[0] |= (m_format & 0x0F);
    m_data[1] |= m_sap << 4;
    m_data[1] |= m_UDTFormat;
    m_data[2] = m_dstId >> 16;
    m_data[3] = (m_dstId >> 8) & 0xFF;
    m_data[4] = (m_dstId & 0xFF);
    m_data[5] = m_srcId >> 16;
    m_data[6] = (m_srcId >> 8) & 0xFF;
    m_data[7] = (m_srcId & 0xFF);
    m_data[8] |= (m_padNibble << 3) & 0xF8;
    m_data[8] |= (m_blocks - 1) & 0x03;
    m_data[9] |= ((unsigned int)m_SF) << 7;
    m_data[9] |= ((unsigned int)m_PF) << 6;
    m_data[9] |= m_opcode;
}

void CDMRDataHeader::setData(unsigned char *data)
{
    ::memset(m_data, 0, 11U);
    for(int i=0;i<10;i++)
    {
        m_data[i] = data[i];
    }
}

bool CDMRDataHeader::getGI() const
{
	return m_GI;
}

void CDMRDataHeader::setGI(bool gi)
{
    m_GI = gi;
}

unsigned char CDMRDataHeader::getRSVD() const
{
    return m_rsvd;
}

void CDMRDataHeader::setRSVD(unsigned char rsvd)
{
    m_rsvd = rsvd;
}

void CDMRDataHeader::setA(bool a)
{
    m_A = a;
}

bool CDMRDataHeader::getA() const
{
    return m_A;
}

void CDMRDataHeader::setFormat(unsigned char format)
{
    m_format = format;
}

unsigned char CDMRDataHeader::getFormat() const
{
    return m_format;
}

void CDMRDataHeader::setUDTFormat(unsigned char udt_format)
{
    m_UDTFormat = udt_format;
}

unsigned char CDMRDataHeader::getUDTFormat() const
{
    return m_UDTFormat;
}

void CDMRDataHeader::setSAP(unsigned char sap)
{
    m_sap = sap;
}

unsigned char CDMRDataHeader::getSAP() const
{
    return m_sap;
}

void CDMRDataHeader::setOpcode(unsigned char opcode)
{
    m_opcode = opcode;
}

unsigned char CDMRDataHeader::getOpcode() const
{
    return m_opcode;
}

void CDMRDataHeader::setSF(bool sf)
{
    m_SF = sf;
}

bool CDMRDataHeader::getSF() const
{
    return m_SF;
}

void CDMRDataHeader::setPF(bool pf)
{
    m_PF = pf;
}

bool CDMRDataHeader::getPF() const
{
    return m_PF;
}

void CDMRDataHeader::setDPF(unsigned char dpf)
{
    m_DPF = dpf;
}

unsigned char CDMRDataHeader::getDPF() const
{
    return m_DPF;
}

unsigned int CDMRDataHeader::getSrcId() const
{
	return m_srcId;
}

void CDMRDataHeader::setSrcId(unsigned int srcId)
{
    m_srcId = srcId;
}

unsigned int CDMRDataHeader::getDstId() const
{
	return m_dstId;
}

void CDMRDataHeader::setDstId(unsigned int dstId)
{
    m_dstId = dstId;
}

unsigned int CDMRDataHeader::getBlocks() const
{
	return m_blocks;
}

void CDMRDataHeader::setBlocks(unsigned int blocks)
{
    m_blocks = blocks;
}

unsigned int  CDMRDataHeader::getPadNibble() const
{
    return m_padNibble;
}

void CDMRDataHeader::setPadNibble(unsigned int pad)
{
    m_padNibble = pad;
}

unsigned int  CDMRDataHeader::getSequenceNumber() const
{
    return m_Ns;
}

bool CDMRDataHeader::getUDT() const
{
    return m_UDT;
}

CDMRDataHeader& CDMRDataHeader::operator=(const CDMRDataHeader& header)
{
	if (&header != this) {
		::memcpy(m_data, header.m_data, 12U);
		m_GI     = header.m_GI;
		m_A      = header.m_A;
		m_srcId  = header.m_srcId;
		m_dstId  = header.m_dstId;
		m_blocks = header.m_blocks;
		m_F      = header.m_F;
		m_S      = header.m_S;
		m_Ns     = header.m_Ns;
        m_UDT    = header.m_UDT;
        m_UDTFormat = header.m_UDTFormat;
        m_rsvd   = header.m_rsvd;
        m_format = header.m_format;
        m_sap    = header.m_sap;
        m_opcode = header.m_opcode;
        m_SF     = header.m_SF;
        m_PF     = header.m_PF;
        m_padNibble = header.m_padNibble;
	}

	return *this;
}
