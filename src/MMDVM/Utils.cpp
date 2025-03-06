/*
 *	Copyright (C) 2009,2014,2015,2016,2021 Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>

void CUtils::dump(const std::string& title, const unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	dump(2U, title, data, length);
}

void CUtils::dump(int level, const std::string& title, const unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	::Log(level, "%s", title.c_str());

	unsigned int offset = 0U;

	while (length > 0U) {
		std::string output;

		unsigned int bytes = (length > 16U) ? 16U : length;

		for (unsigned i = 0U; i < bytes; i++) {
			char temp[10U];
			::sprintf(temp, "%02X ", data[offset + i]);
			output += temp;
		}

		for (unsigned int i = bytes; i < 16U; i++)
			output += "   ";

		output += "   *";

		for (unsigned i = 0U; i < bytes; i++) {
			unsigned char c = data[offset + i];

			if (::isprint(c))
				output += c;
			else
				output += '.';
		}

		output += '*';

		::Log(level, "%04X:  %s", offset, output.c_str());

		offset += 16U;

		if (length >= 16U)
			length -= 16U;
		else
			length = 0U;
	}
}

void CUtils::dump(const std::string& title, const bool* bits, unsigned int length)
{
	assert(bits != NULL);

	dump(2U, title, bits, length);
}

void CUtils::dump(int level, const std::string& title, const bool* bits, unsigned int length)
{
	assert(bits != NULL);

	unsigned char bytes[100U];
	unsigned int nBytes = 0U;
	for (unsigned int n = 0U; n < length; n += 8U, nBytes++)
		bitsToByteBE(bits + n, bytes[nBytes]);

	dump(level, title, bytes, nBytes);
}

void CUtils::byteToBitsBE(unsigned char byte, bool* bits)
{
	assert(bits != NULL);

	bits[0U] = (byte & 0x80U) == 0x80U;
	bits[1U] = (byte & 0x40U) == 0x40U;
	bits[2U] = (byte & 0x20U) == 0x20U;
	bits[3U] = (byte & 0x10U) == 0x10U;
	bits[4U] = (byte & 0x08U) == 0x08U;
	bits[5U] = (byte & 0x04U) == 0x04U;
	bits[6U] = (byte & 0x02U) == 0x02U;
	bits[7U] = (byte & 0x01U) == 0x01U;
}

void CUtils::byteToBitsLE(unsigned char byte, bool* bits)
{
	assert(bits != NULL);

	bits[0U] = (byte & 0x01U) == 0x01U;
	bits[1U] = (byte & 0x02U) == 0x02U;
	bits[2U] = (byte & 0x04U) == 0x04U;
	bits[3U] = (byte & 0x08U) == 0x08U;
	bits[4U] = (byte & 0x10U) == 0x10U;
	bits[5U] = (byte & 0x20U) == 0x20U;
	bits[6U] = (byte & 0x40U) == 0x40U;
	bits[7U] = (byte & 0x80U) == 0x80U;
}

void CUtils::bitsToByteBE(const bool* bits, unsigned char& byte)
{
	assert(bits != NULL);

	byte  = bits[0U] ? 0x80U : 0x00U;
	byte |= bits[1U] ? 0x40U : 0x00U;
	byte |= bits[2U] ? 0x20U : 0x00U;
	byte |= bits[3U] ? 0x10U : 0x00U;
	byte |= bits[4U] ? 0x08U : 0x00U;
	byte |= bits[5U] ? 0x04U : 0x00U;
	byte |= bits[6U] ? 0x02U : 0x00U;
	byte |= bits[7U] ? 0x01U : 0x00U;
}

void CUtils::bitsToByteLE(const bool* bits, unsigned char& byte)
{
	assert(bits != NULL);

	byte  = bits[0U] ? 0x01U : 0x00U;
	byte |= bits[1U] ? 0x02U : 0x00U;
	byte |= bits[2U] ? 0x04U : 0x00U;
	byte |= bits[3U] ? 0x08U : 0x00U;
	byte |= bits[4U] ? 0x10U : 0x00U;
	byte |= bits[5U] ? 0x20U : 0x00U;
	byte |= bits[6U] ? 0x40U : 0x00U;
	byte |= bits[7U] ? 0x80U : 0x00U;
}

unsigned int CUtils::countBits(unsigned int v)
{
	unsigned int count = 0U;

	while (v != 0U) {
		v &= v - 1U;
		count++;
	}

	return count;
}

void CUtils::removeChar(unsigned char * haystack, char needdle)
{
    unsigned int i = 0;
	unsigned int j = 0;

    while (haystack[i] != '\0') {
        if (haystack[i] != needdle)
            haystack[j++] = haystack[i];
		i++;
	}
 
    haystack[j] = '\0';
}

void CUtils::extractGPSPosition(unsigned char* data, std::string &error, float &longitude, float &latitude)
{
    unsigned int errorI = (data[2U] & 0x0E) >> 1U;

    switch (errorI) {
    case 0U:
        error = "< 2m";
        break;
    case 1U:
        error = "< 20m";
        break;
    case 2U:
        error = "< 200m";
        break;
    case 3U:
        error = "< 2km";
        break;
    case 4U:
        error = "< 20km";
        break;
    case 5U:
        error = "< 200km";
        break;
    case 6U:
        error = "> 200km";
        break;
    default:
        error = "not known";
        break;
    }

    int32_t longitudeI = ((data[2U] & 0x01U) << 31) | (data[3U] << 23) | (data[4U] << 15) | (data[5U] << 7);
    longitudeI >>= 7;

    int32_t latitudeI = (data[6U] << 24) | (data[7U] << 16) | (data[8U] << 8);
    latitudeI >>= 8;

    longitude = 360.0F / 33554432.0F;	// 360/2^25 steps
    latitude  = 180.0F / 16777216.0F;	// 180/2^24 steps

    longitude *= float(longitudeI);
    latitude  *= float(latitudeI);
}

