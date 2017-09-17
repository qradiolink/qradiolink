#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QString>

class VideoEncoder
{
public:
    VideoEncoder(QString device_name="/dev/video0");
    ~VideoEncoder();
    void encode_jpeg(unsigned char *videobuffer, unsigned long &encoded_size);
    unsigned char *decode_jpeg(unsigned char *videobuffer, int data_length);

private:

};

#endif // VIDEOENCODER_H
