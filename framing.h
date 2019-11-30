#ifndef FRAMING_H
#define FRAMING_H

#include <QMap>
#include <QVector>

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

    union frame_map
    {
        int frame_type;
        char bytes[4];
    };
}



#endif // FRAMING_H
