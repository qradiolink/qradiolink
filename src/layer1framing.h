#ifndef LAYER1FRAMING_H
#define LAYER1FRAMING_H

#include <QObject>
#include <QByteArray>


enum frame_type
{
    FrameTypeNone = 0x00,
    FrameTypeVoice = 0xED89, // legacy
    FrameTypeVoice2 = 0xED89,
    FrameTypeVoice1 = 0xB5,
    FrameTypeText = 0x89EDAA,
    FrameTypeIP = 0xDE98AA,
    FrameTypeVideo = 0x98DEAA,
    FrameTypeSync = 0xCC,
    FrameTypeCallsign = 0x8CC8DD,
    FrameTypeProto = 0xED77AA,
    FrameTypeEnd = 0x4C8A2B,
};

class Layer1Framing : public QObject
{
    Q_OBJECT
public:
    explicit Layer1Framing(QObject *parent = nullptr);
    QByteArray getFrameHeader(int frame_type);

signals:

public slots:
};

#endif // LAYER1FRAMING_H
