#include "layer1framing.h"

Layer1Framing::Layer1Framing(QObject *parent) : QObject(parent)
{

}


/// Assembles frame header
QByteArray Layer1Framing::getFrameHeader(int frame_type)
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
