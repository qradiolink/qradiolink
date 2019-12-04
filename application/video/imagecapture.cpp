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

#include "imagecapture.h"

ImageCapture::ImageCapture(Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _max_video_frame_size = 3122;
    _inited = false;

    /// Large alloc
    _videobuffer = (unsigned char*)calloc(_max_video_frame_size,
                                                        sizeof(unsigned char));
}

ImageCapture::~ImageCapture()
{
    delete[] _videobuffer;
}

void ImageCapture::init()
{
    if(_inited)
    {
        deinit();
    }
    QCameraInfo camera_info = QCameraInfo::defaultCamera();
    _camera = new QCamera(camera_info);
    _camera->setCaptureMode(QCamera::CaptureStillImage);
    _camera->exposure()->setAutoAperture();
    _camera->exposure()->setAutoIsoSensitivity();
    _camera->exposure()->setExposureMode(QCameraExposure::ExposureAuto);
    _camera->exposure()->setManualShutterSpeed(0.01);
    _capture = new QCameraImageCapture(_camera);
    _capture->setBufferFormat(QVideoFrame::Format_Jpeg);
    _capture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    QObject::connect(_capture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(process_image(int,QImage)));
    QImageEncoderSettings encoding_settings;
    encoding_settings.setResolution(320, 240);
    encoding_settings.setCodec("image/jpeg");
    encoding_settings.setQuality(QMultimedia::VeryLowQuality);
    _capture->setEncodingSettings(encoding_settings);
    _camera->start();
    _inited = true;
}

void ImageCapture::deinit()
{
    if(!_inited)
        return;
    QObject::disconnect(_capture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(process_image()));
    delete _capture;
    delete _camera;
}

void ImageCapture::capture_image()
{
    _camera->searchAndLock();
    _capture->capture();
    _camera->unlock();
}

void ImageCapture::process_image(int id, QImage img)
{
    qDebug() << img.size();
}
