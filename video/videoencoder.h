// Written by Adrian Musceac YO8RZZ , started March 2016.
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

#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QString>
#include <QDebug>
#include <QDateTime>
#include <iostream>
#include "logger.h"

class VideoEncoder
{
public:
    VideoEncoder(Logger *logger);
    ~VideoEncoder();
    void init(QString device_name);
    void deinit();
    void encode_jpeg(unsigned char *videobuffer, unsigned long &encoded_size, unsigned long max_video_frame_size);
    unsigned char *decode_jpeg(unsigned char *videobuffer, int data_length);

private:
    Logger *_logger;
    bool _init;

};

#endif // VIDEOENCODER_H
