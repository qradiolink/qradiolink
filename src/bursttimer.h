// Written by Adrian Musceac YO8RZZ , started March 2021.
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

#ifndef BURSTTIMER_H
#define BURSTTIMER_H

#include <mutex>
#include <chrono>
#include <QVector>
#include "src/config_mmdvm.h"

#define MAX_MMDVM_CHANNELS 7

static const uint64_t BURST_DELAY = 100000000L; // nanosec
static const uint64_t SLOT_TIME = 30000000L;
static const int64_t TX_TIMEOUT = 50000L;
static const int32_t SAMPLES_PER_SLOT = 720;

// FIFO fill count
extern int lime_fifo_fill_count;

class BurstTimer
{
public:
    BurstTimer(uint64_t burst_delay=BURST_DELAY, uint64_t samples_per_slot=720, uint64_t time_per_sample=41667L,
               uint64_t slot_time=SLOT_TIME);
    ~BurstTimer();
    void set_enabled(bool value);
    void set_params(uint64_t samples_per_slot, uint64_t time_per_sample, uint64_t slot_time, uint64_t burst_delay);
    void reset_timer(int cn=0);
    uint64_t get_time_delta(int cn=0);
    void set_timer(uint64_t value, int cn=0);
    void increment_sample_counter(int cn=0);
    uint64_t get_sample_counter(int cn);
    int check_time(int cn=0);
    uint64_t allocate_slot(int slot_no, int cn=0);
    uint64_t get_last_timestamp(int cn=0);
    uint64_t get_last_timestamp_global();
    void set_last_timestamp(int cn, uint64_t value);
    void set_tx(int cn, bool value, bool wait_timeout=true);
    bool get_tx(int cn=0);
    bool get_other_tx_status(int cn);
    bool get_global_tx_status();

private:
    bool _enabled;
    bool _tx[MAX_MMDVM_CHANNELS];
    std::mutex _timing_mutex[MAX_MMDVM_CHANNELS];
    std::mutex _slot_mutex[MAX_MMDVM_CHANNELS];
    std::mutex _last_timestamp_mutex[MAX_MMDVM_CHANNELS];
    std::mutex _tx_mutex[MAX_MMDVM_CHANNELS];
    struct slot {
        uint8_t slot_no;
        uint64_t slot_time;
        uint64_t slot_sample_counter;
    };
    uint64_t _samples_per_slot;
    uint64_t _time_per_sample;
    uint64_t _slot_time;
    uint64_t _burst_delay;
    uint64_t _sample_counter[MAX_MMDVM_CHANNELS];
    uint64_t _last_slot[MAX_MMDVM_CHANNELS];
    uint64_t _time_base[MAX_MMDVM_CHANNELS];
    uint64_t _last_timestamp[MAX_MMDVM_CHANNELS];
    std::chrono::high_resolution_clock::time_point t1[MAX_MMDVM_CHANNELS];
    std::chrono::high_resolution_clock::time_point t2[MAX_MMDVM_CHANNELS];
    std::chrono::high_resolution_clock::time_point tx1[MAX_MMDVM_CHANNELS];
    std::chrono::high_resolution_clock::time_point tx2[MAX_MMDVM_CHANNELS];
    QVector<slot*> _slot_times[MAX_MMDVM_CHANNELS];

};


#endif // BURSTTIMER_H
