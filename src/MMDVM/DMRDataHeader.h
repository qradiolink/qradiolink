/*
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

#ifndef DMRDataHeader_H
#define DMRDataHeader_H

enum UDTOpcode {
    C_UDTHD = 0x1A,
    C_UDTHU = 0x1B,
    C_DGNAHD = 0x24,
};

class CDMRDataHeader
{
public:
	CDMRDataHeader();
	~CDMRDataHeader();

	bool put(const unsigned char* bytes);
    void construct();
    void setData(unsigned char *data);

	void get(unsigned char* bytes) const;

	bool          getGI() const;
    void          setGI(bool gi);

    unsigned char getRSVD() const;
    void          setRSVD(unsigned char rsvd);

    bool          getA() const;
    void          setA(bool a);

    void          setFormat(unsigned char format);
    unsigned char getFormat() const;

    void          setUDTFormat(unsigned char udt_format);
    unsigned char getUDTFormat() const;

    void          setSAP(unsigned char sap);
    unsigned char getSAP() const;

    void          setOpcode(unsigned char opcode);
    unsigned char getOpcode() const;

    void          setSF(bool sf);
    bool          getSF() const;

    void          setPF(bool pf);
    bool          getPF() const;

    void          setDPF(unsigned char dpf);
    unsigned char getDPF() const;

	unsigned int  getSrcId() const;
	unsigned int  getDstId() const;
    void          setSrcId(unsigned int srcId);
    void          setDstId(unsigned int dstId);

	unsigned int  getBlocks() const;
    void          setBlocks(unsigned int blocks);
    unsigned int  getPadNibble() const;
    void          setPadNibble(unsigned int pad);
    unsigned int  getSequenceNumber() const;
    bool          getUDT() const;

	CDMRDataHeader& operator=(const CDMRDataHeader& header);

private:
	unsigned char* m_data;
	bool           m_GI;
	bool           m_A;
	unsigned int   m_srcId;
	unsigned int   m_dstId;
	unsigned int   m_blocks;
    unsigned int   m_padNibble;
	bool           m_F;
	bool           m_S;
	unsigned char  m_Ns;
    bool           m_UDT;
    unsigned char  m_UDTFormat;
    unsigned char  m_rsvd;
    unsigned char  m_format;
    unsigned char  m_sap;
    unsigned char  m_opcode;
    bool           m_SF;
    bool           m_PF;
    unsigned char  m_DPF;
};

#endif

