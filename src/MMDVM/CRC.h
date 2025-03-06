/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#if !defined(CRC_H)
#define	CRC_H

class CCRC
{
public:
	static bool checkFiveBit(bool* in, unsigned int tcrc);
	static void encodeFiveBit(const bool* in, unsigned int& tcrc);

	static void addCCITT161(unsigned char* in, unsigned int length);
	static void addCCITT162(unsigned char* in, unsigned int length);

	static bool checkCCITT161(const unsigned char* in, unsigned int length);
	static bool checkCCITT162(const unsigned char* in, unsigned int length);

	static unsigned char crc8(const unsigned char* in, unsigned int length);
};

#endif
