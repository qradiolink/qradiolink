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

#ifndef IMAGECAPTURE_H
#define IMAGECAPTURE_H

#include <QObject>
#include <QCamera>
#include <QCameraInfo>
#include <QCameraImageCapture>
#include "settings.h"

class ImageCapture : public QObject
{
    Q_OBJECT
public:
    explicit ImageCapture(Settings *settings, QObject *parent = nullptr);
    ~ImageCapture();

    void init();
    void deinit();
    void capture_image(unsigned char *frame, int &len);

signals:

public slots:
    void process_image();

private:
    Settings *_settings;
    QCamera *_camera;
    QCameraImageCapture *_capture;
};

#endif // IMAGECAPTURE_H
