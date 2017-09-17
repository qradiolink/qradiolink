#include "videoencoder.h"

#include "video/videocapture.cpp"
extern "C"
{
#include <jpeglib.h>
}

VideoEncoder::VideoEncoder()
{
    dev_name = "/dev/video0";
    open_device();
    init_device();
    start_capturing();
}

VideoEncoder::~VideoEncoder()
{
    stop_capturing();
    uninit_device();
    close_device();
}

void VideoEncoder::encode_jpeg(unsigned char *videobuffer, unsigned long &encoded_size)
{
    int len;
    char *frame = new char[614400];
    capture_frame(frame, len);
    char *input = frame;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_ptr[1];
    int row_stride;

    encoded_size = 0;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &videobuffer, &encoded_size);

        // jrow is a libjpeg row of samples array of 1 row pointer
    cinfo.image_width = 640 & -1;
    cinfo.image_height = 480 & -1;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr; //libJPEG expects YUV 3bytes, 24bit

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 8, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    unsigned char tmprowbuf[640 * 3];

    JSAMPROW row_pointer[1];
    row_pointer[0] = &tmprowbuf[0];
    while (cinfo.next_scanline < cinfo.image_height) {
        unsigned i, j;
        unsigned offset = cinfo.next_scanline * cinfo.image_width * 2; //offset to the correct row
        for (i = 0, j = 0; i < cinfo.image_width * 2; i += 4, j += 6) { //input strides by 4 bytes, output strides by 6 (2 pixels)
            tmprowbuf[j + 0] = input[offset + i + 0]; // Y (unique to this pixel)
            tmprowbuf[j + 1] = input[offset + i + 1]; // U (shared between pixels)
            tmprowbuf[j + 2] = input[offset + i + 3]; // V (shared between pixels)
            tmprowbuf[j + 3] = input[offset + i + 2]; // Y (unique to this pixel)
            tmprowbuf[j + 4] = input[offset + i + 1]; // U (shared between pixels)
            tmprowbuf[j + 5] = input[offset + i + 3]; // V (shared between pixels)
        }
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    delete[] frame;
}
