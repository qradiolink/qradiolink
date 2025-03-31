/*
 *   Copyright (C) 2015,2020 by Jonathan Naylor G4KLX
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
#include <stdint.h>


#if !defined(DMRSLOTTYPE2_H)
#define  DMRSLOTTYPE2_H

class CDMRSlotType2 {
public:
  CDMRSlotType2();

  void decode(const uint8_t* frame, uint8_t& colorCode, uint8_t& dataType) const;

  void encode(uint8_t colorCode, uint8_t dataType, uint8_t* frame) const;

private:

  uint8_t  decode2087(const uint8_t* data) const;
  uint32_t getSyndrome1987(uint32_t pattern) const;
};


#endif

