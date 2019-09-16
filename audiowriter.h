#ifndef AUDIOWRITER_H
#define AUDIOWRITER_H

#include <QObject>
#include <vector>
#include <QCoreApplication>
#include <QMutex>
#include "audio/audiointerface.h"

class AudioWriter : public QObject
{
    Q_OBJECT
public:
    explicit AudioWriter(QObject *parent = 0);
    ~AudioWriter();

signals:
    void finished();

public slots:
    void run();
    void writePCM(short *pcm, int bytes, bool preprocess, int audio_mode);
    void stop();

private:
    struct audio_samples
    {
        audio_samples() : pcm(0), bytes(0), preprocess(false), audio_mode(0) {}
        short *pcm;
        int bytes;
        bool preprocess;
        int audio_mode;
    };

    AudioInterface *_audio_writer;
    std::vector<audio_samples*> *_rx_sample_queue;
    //std::vector<audio_samples*> *_tx_sample_queue;
    bool _working;
    QMutex _mutex;

};

#endif // AUDIOWRITER_H
