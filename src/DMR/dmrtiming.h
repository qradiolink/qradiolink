// Written by Adrian Musceac YO8RZZ , started Oct 2024.
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

#ifndef DMRTIMING_H
#define DMRTIMING_H

#include <QObject>
#include <mutex>
#include <chrono>
#include <QVector>
#include "src/bursttimer.h"
#include "src/settings.h"
#include "src/DMR/constants.h"
#include "src/DMR/dmrcontrol.h"

class DMRTiming : public QObject
{
    Q_OBJECT
public:
    explicit DMRTiming(const Settings *settings, uint64_t samples_per_slot=SAMPLES_PER_SLOT, uint64_t time_per_sample=TIME_PER_SAMPLE,
                       uint64_t slot_time=SLOT_TIME, QObject *parent = nullptr);

    void set_params(uint64_t samples_per_slot, uint64_t time_per_sample, uint64_t slot_time);
    void reset_timer();
    void set_timer(uint64_t value);
    void set_tx_time(bool value);
    bool get_tx_time();
    void increment_sample_counter(uint64_t no_of_samples);
    uint64_t get_sample_counter();
    void set_slot_times(uint8_t sn);
    uint64_t get_slot_times(uint8_t sn);
    bool timing_recent(uint8_t sn);

signals:
    void timing_ready(unsigned int sn);

private:
    const Settings *_settings;
    std::mutex _timing_mutex;
    std::mutex _slot_mutex[NUMBER_OF_SLOTS];
    std::mutex _last_timestamp_mutex[NUMBER_OF_SLOTS];
    std::mutex _tx_mutex;
    uint64_t _samples_per_slot;
    uint64_t _time_per_sample;
    uint64_t _slot_time;
    uint64_t _slot_times[NUMBER_OF_SLOTS];
    uint64_t _sample_counter;
    uint64_t _time_base;
    uint64_t _next_tx_time;
    bool _tx;
    bool _first;
    std::chrono::high_resolution_clock::time_point t1;
    std::chrono::high_resolution_clock::time_point t2;


};

#endif // DMRTIMING_H
