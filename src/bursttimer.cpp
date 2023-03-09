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

#include "bursttimer.h"
#include <QDebug>

int lime_fifo_fill_count = 50;

BurstTimer::BurstTimer(uint64_t burst_delay, uint64_t samples_per_slot, uint64_t time_per_sample,
                       uint64_t slot_time)
{
    _enabled = true;
    _samples_per_slot = samples_per_slot;
    _time_per_sample = time_per_sample;
    _slot_time = slot_time;
    _burst_delay = burst_delay;
    for(uint8_t i = 0;i < MAX_MMDVM_CHANNELS;i++)
        _sample_counter[i] = 0;
    for(uint8_t i = 0;i < MAX_MMDVM_CHANNELS;i++)
        _last_slot[i] = 0;
    for(uint8_t i = 0;i < MAX_MMDVM_CHANNELS;i++)
        _time_base[i] = 0;
    for(uint8_t i = 0;i < MAX_MMDVM_CHANNELS;i++)
        _last_timestamp[i] = 0;
    for(uint8_t i = 0;i < MAX_MMDVM_CHANNELS;i++)
        _tx[i] = false;
    for(uint8_t i = 0;i < MAX_MMDVM_CHANNELS;i++)
        t1[i] = std::chrono::high_resolution_clock::now();
    for(uint8_t i = 0;i < MAX_MMDVM_CHANNELS;i++)
        tx1[i] = std::chrono::high_resolution_clock::now();
}

BurstTimer::~BurstTimer()
{
    for(uint8_t k = 0;k < MAX_MMDVM_CHANNELS;k++)
    {
        for(int i=0;i<_slot_times[k].size();i++)
        {
            delete _slot_times[k].at(i);
        }
    }
}

void BurstTimer::set_enabled(bool value)
{
    _enabled = value;
}

void BurstTimer::set_tx(int cn, bool value, bool wait_timeout)
{
    std::unique_lock<std::mutex> guard(_tx_mutex[cn]);
    if(value)
    {
        tx1[cn] = std::chrono::high_resolution_clock::now();
        _tx[cn] = value;
    }
    else
    {
        if(wait_timeout)
        {
            tx2[cn] = std::chrono::high_resolution_clock::now();
            if(std::chrono::duration_cast<std::chrono::nanoseconds>(tx2[cn]-tx1[cn]).count() > (int64_t)TX_TIMEOUT)
                _tx[cn] = value;
        }
        else
        {
            _tx[cn] = value;
        }
    }
}

bool BurstTimer::get_tx(int cn)
{
    std::unique_lock<std::mutex> guard(_tx_mutex[cn]);
    return _tx[cn];
}

bool BurstTimer::get_other_tx_status(int cn)
{
    bool tx_status = false;
    for(int i = 0;i < MAX_MMDVM_CHANNELS;i++)
    {
        if(i == cn)
            continue;
        std::unique_lock<std::mutex> guard(_tx_mutex[i]);
        if(_tx[i])
            tx_status = true;
        guard.unlock();
    }
    return tx_status;
}

bool BurstTimer::get_global_tx_status()
{
    bool tx_status = false;
    for(int i = 0;i < MAX_MMDVM_CHANNELS;i++)
    {
        std::unique_lock<std::mutex> guard(_tx_mutex[i]);
        if(_tx[i])
            tx_status = true;
        guard.unlock();
    }
    return tx_status;
}

void BurstTimer::set_last_timestamp(int cn, uint64_t value)
{
    std::unique_lock<std::mutex> guard(_last_timestamp_mutex[cn]);
    _last_timestamp[cn] = value;
}

uint64_t BurstTimer::get_last_timestamp(int cn)
{
    std::unique_lock<std::mutex> guard(_last_timestamp_mutex[cn]);
    return _last_timestamp[cn];

}

uint64_t BurstTimer::get_last_timestamp_global()
{
    uint64_t last_timestamp = 0;
    for(int i = 0;i < MAX_MMDVM_CHANNELS;i++)
    {
        std::unique_lock<std::mutex> guard(_last_timestamp_mutex[i]);
        if(_last_timestamp[i] > last_timestamp)
            last_timestamp = _last_timestamp[i];
    }

    return last_timestamp;

}

void BurstTimer::set_params(uint64_t samples_per_slot, uint64_t time_per_sample,
                            uint64_t slot_time, uint64_t burst_delay)
{
    _samples_per_slot = samples_per_slot;
    _time_per_sample = time_per_sample;
    _slot_time = slot_time;
    _burst_delay = burst_delay;
}

uint64_t BurstTimer::get_time_delta(int cn)
{
    std::unique_lock<std::mutex> guard(_timing_mutex[cn]);
    t2[cn] = std::chrono::high_resolution_clock::now();
    return _time_base[cn] + (uint64_t) std::chrono::duration_cast<std::chrono::nanoseconds>(t2[cn]-t1[cn]).count();
}

void BurstTimer::reset_timer(int cn)
{
    std::unique_lock<std::mutex> guard(_timing_mutex[cn]);
    _sample_counter[cn] = 0;
    _time_base[cn] = 0;
    t1[cn]= std::chrono::high_resolution_clock::now();
    //qDebug() << "================= Restarted burst timer =======================";
}

void BurstTimer::set_timer(uint64_t value, int cn)
{
    std::unique_lock<std::mutex> guard(_timing_mutex[cn]);
    //qDebug() << "================= Set timer: " << value << " ===================";
    _sample_counter[cn] = 0;
    _time_base[cn] = value;
    t1[cn] = std::chrono::high_resolution_clock::now();
}

void BurstTimer::increment_sample_counter(int cn)
{
    std::unique_lock<std::mutex> guard(_timing_mutex[cn]);
    _sample_counter[cn]++;
}

uint64_t BurstTimer::get_sample_counter(int cn)
{
    std::unique_lock<std::mutex> guard(_timing_mutex[cn]);
    return _time_base[cn] + _sample_counter[cn];
}


int BurstTimer::check_time(int cn)
{
    if(!_enabled)
        return 0;
    slot *s;
    std::unique_lock<std::mutex> guard(_slot_mutex[cn]);
    if(_slot_times[cn].size() < 1)
        return 0;
    s = _slot_times[cn][0];
    std::unique_lock<std::mutex> guard_time(_timing_mutex[cn]);
    uint64_t sample_time = _time_base[cn] + _sample_counter[cn] * _time_per_sample;
    guard_time.unlock();
    if(sample_time >= s->slot_time && s->slot_sample_counter == 0)
    {
        s->slot_sample_counter++;
        return s->slot_no;
    }
    else if(sample_time >= s->slot_time)
    {
        if(s->slot_sample_counter >= (_samples_per_slot - 1))
        {
            delete _slot_times[cn][0];
            _slot_times[cn].removeFirst();
            //qDebug() << "============= Slots remaining: " << _slot_times[cn].size();
            return 0;
        }
        s->slot_sample_counter++;
    }
    return 0;
}

uint64_t BurstTimer::allocate_slot(int slot_no, int cn)
{
    if(!_enabled)
        return 0L;
    slot *s = new slot;
    s->slot_no = (uint8_t)slot_no;
    uint64_t elapsed = get_time_delta(cn);
    //uint64_t elapsed = _time_base[cn] + _sample_counter[cn] * _time_per_sample;

    if(elapsed <= _last_slot[cn])
    {
        _last_slot[cn] = _last_slot[cn] + _slot_time;
    }
    else if(_last_slot[cn] == 0)
    {
        _last_slot[cn] = elapsed;
    }
    else if((elapsed - _last_slot[cn]) >= (1L * _slot_time))
    {
        _last_slot[cn] = elapsed;
    }
    else
    {
        _last_slot[cn] = _last_slot[cn] + _slot_time;
    }
    uint64_t nsec = _last_slot[cn] + _burst_delay;
    s->slot_time = nsec;
    s->slot_sample_counter = 0;
    std::unique_lock<std::mutex> guard(_slot_mutex[cn]);
    _slot_times[cn].append(s);

    /// send with timestamp in advance with X nanoseconds
    /// this accounts for delay in RF stage
    return nsec;
}



