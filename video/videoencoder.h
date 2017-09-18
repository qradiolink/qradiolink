#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QString>
#include <QDebug>
#include <QDateTime>

class VideoEncoder
{
public:
    VideoEncoder(QString device_name="/dev/video0");
    ~VideoEncoder();
    void encode_jpeg(unsigned char *videobuffer, unsigned long &encoded_size, int max_video_frame_size);
    unsigned char *decode_jpeg(unsigned char *videobuffer, int data_length);

private:

};

#endif // VIDEOENCODER_H
