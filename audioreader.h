#ifndef AUDIOREADER_H
#define AUDIOREADER_H

#include <QCoreApplication>
#include <QMutex>
#include "audio/audiointerface.h"

class AudioReader : public QObject
{
    Q_OBJECT
public:
    explicit AudioReader(QObject *parent = 0);

signals:
    void finished();
    void audioPCM(short *pcm, int bytes, int vad, bool radio_only);

public slots:
    void run();
    void setReadMode(bool capture, bool preprocess, int audio_mode);
    void stop();

private:
    AudioInterface *_audio_reader;
    bool _working;
    bool _capture_audio;
    bool _read_preprocess;
    int _read_audio_mode;
    QMutex _mutex;


};

#endif // AUDIOREADER_H
