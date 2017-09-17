#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H



class VideoEncoder
{
public:
    VideoEncoder();
    ~VideoEncoder();
    void encode_jpeg(unsigned char *videobuffer, unsigned long &encoded_size);
    short *decode_jpeg(unsigned char *audiobuffer, int data_length, int &samples);

private:

};

#endif // VIDEOENCODER_H
