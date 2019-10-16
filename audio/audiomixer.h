// Written by Adrian Musceac YO8RZZ , started October 2013.
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

#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H

#include <QObject>
#include <QDebug>
#include <QVector>
#include <QMap>
#include <QMutex>

class AudioMixer : public QObject
{
    Q_OBJECT
public:
    explicit AudioMixer(QObject *parent = nullptr);
    ~AudioMixer();

signals:

public slots:
    void addSamples(short *pcm, int samples, int sid);
    short *mix_samples(float rx_volume);
    bool buffers_available();
    void empty();

private:
    QMap<int, QVector<short>*> _sample_buffers;
    QMutex _mutex;

};

#endif // AUDIOMIXER_H
