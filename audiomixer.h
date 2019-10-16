#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H

#include <QObject>
#include <QDebug>
#include <QVector>
#include <QMap>

class AudioMixer : public QObject
{
    Q_OBJECT
public:
    explicit AudioMixer(QObject *parent = nullptr);

signals:

public slots:
    void addSamples(short *pcm, int samples, int sid);
    short *mix_samples();

private:
    QMap<int, QVector<short>*> _sample_buffers;

};

#endif // AUDIOMIXER_H
