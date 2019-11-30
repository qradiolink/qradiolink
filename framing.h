// Written by Adrian Musceac YO8RZZ , started October 2019.
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

#ifndef FRAMING_H
#define FRAMING_H

#include <QByteArray>

namespace modem_framing {

    enum frame_type
    {
        FrameTypeNone = 0x00,
        FrameTypeVoice = 0xED89,
        FrameTypeVoice2 = 0xED89,
        FrameTypeVoice1 = 0xB5,
        FrameTypeText = 0x89EDAA,
        FrameTypeData = 0xDE98AA,
        FrameTypeVideo = 0x98DEAA,
        FrameTypeSync = 0xCC,
        FrameTypeCallsign = 0x8CC8DD,
        FrameTypeProto = 0xED77AA,
        FrameTypeEnd = 0x4C8A2B,
    };

    /// Assembles frame header
    inline QByteArray getFrameHeader(int frame_type)
    {
        QByteArray header;
        unsigned char c[4];
        c[0] = frame_type & 0xFF;
        c[1] = (frame_type>>8) & 0xFF;
        c[2] = (frame_type>>16) & 0xFF;
        c[3] = (frame_type>>24) & 0xFF;
        for(int i=3;i >= 0;i--)
        {
            if(c[i] != 0x0)
            {
                header.append(c[i]);
            }
        }
        return header;
    }
}



#endif // FRAMING_H
