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
#define FRAME_SIZE 230400 // 320 x 240 x 3 (RGB)

ImageCapture::ImageCapture(Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _inited = false;
    _shutdown = false;
    _capturing = false;
    _last_frame_length = 0;
    _videobuffer = new unsigned char[FRAME_SIZE];
    memset(_videobuffer, 0, FRAME_SIZE*sizeof(unsigned char));
}

ImageCapture::~ImageCapture()
{
    deinit();
    delete[] _videobuffer;
}

void ImageCapture::init()
{
    _mutex.lock();
    if(_inited)
    {
        _mutex.unlock();
        return;
    }
    _mutex.unlock();
    if (QCameraInfo::availableCameras().count() < 1)
    {
            _logger->log(Logger::LogLevelCritical, QString("No available camera found"));
            return;
    }
    QCameraInfo camera_info = QCameraInfo::defaultCamera();
    _camera = new QCamera(camera_info);
    _camera->setCaptureMode(QCamera::CaptureStillImage);
    _camera->exposure()->setAutoAperture();
    _camera->exposure()->setAutoIsoSensitivity();
    _camera->exposure()->setExposureMode(QCameraExposure::ExposureAuto);
    _camera->exposure()->setManualShutterSpeed(0.009);
    _capture = new QCameraImageCapture(_camera);
    _capture->setBufferFormat(QVideoFrame::Format_RGB24);
    _capture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    QObject::connect(_capture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(process_image(int,QImage)));
    QImageEncoderSettings encoding_settings;
    encoding_settings.setResolution(320, 240);
    encoding_settings.setCodec("");
    //encoding_settings.setQuality(QMultimedia::VeryLowQuality);
    _capture->setEncodingSettings(encoding_settings);
    //QWidget *w = QApplication::activeWindow();
    //_viewfinder = new QCameraViewfinder(w);
    //_viewfinder->moveToThread(QCoreApplication::instance()->thread());
    //_camera->setViewfinder(_viewfinder);
    //_viewfinder->show();
    //_viewfinder->raise();
    _camera->start();
    _mutex.lock();
    _inited = true;
    _mutex.unlock();
}

void ImageCapture::deinit()
{
    _mutex.lock();
    if(!_inited)
    {
        _mutex.unlock();
        return;
    }
    //_viewfinder->hide();
    //delete _viewfinder;
    _shutdown = true;
    while(_capturing)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
    }

    _capture->cancelCapture();
    QObject::disconnect(_capture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(process_image(int,QImage)));
    _camera->stop();
    while(1)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        if(_camera->state() != QCamera::State::ActiveState)
            break;
    }
    _camera->unload();
    while(1)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        if(_camera->state() == QCamera::State::UnloadedState)
            break;
    }
    _capture->deleteLater();
    _camera->deleteLater();

    _inited = false;
    _shutdown = false;
    _mutex.unlock();
}

void ImageCapture::capture_image()
{
    _mutex.lock();
    if((!_inited) || (_shutdown) || (_capturing))
    {
        _mutex.unlock();
        return;
    }

    _capturing = true;
    _camera->searchAndLock();
    _capture->capture();
    _camera->unlock();
    _capturing = false;
    _mutex.unlock();
}

void ImageCapture::process_image(int id, QImage img)
{
    Q_UNUSED(id);
    img = img.convertToFormat(QImage::Format_RGB888);
    unsigned char *data = (unsigned char*)img.bits();
    _last_frame_length = img.sizeInBytes();
    memcpy(_videobuffer, data, _last_frame_length);
}

unsigned char* ImageCapture::get_frame(int &len)
{
    if(!_inited)
    {
        len = 0;
        return nullptr;
    }
    capture_image();
    len = _last_frame_length;
    if(len == 0)
        return nullptr;
    unsigned char* frame = new unsigned char[FRAME_SIZE];
    memcpy(frame, _videobuffer, FRAME_SIZE*sizeof(unsigned char));
    return frame;
}
