#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H



class VideoEncoder
{
public:
    VideoEncoder();
    ~VideoEncoder();
    unsigned char *encode_jpeg(short *audiobuffer, int audiobuffersize, int &encoded_size);
    short *decode_jpeg(unsigned char *audiobuffer, int data_length, int &samples);

private:

};

#endif // VIDEOENCODER_H
