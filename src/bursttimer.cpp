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


BurstTimer::BurstTimer(uint64_t samples_per_slot, uint64_t time_per_sample,
                       uint64_t slot_time, uint64_t burst_delay)
{
    _enabled = true;
    _samples_per_slot = samples_per_slot;
    _time_per_sample = time_per_sample;
    _slot_time = slot_time;
    _burst_delay = burst_delay;
    _sample_counter = 0;
    _last_slot[0] = 0;
    _last_slot[1] = 0;
    _time_base = 0;
    _last_timestamp = 0;

    t1 = std::chrono::high_resolution_clock::now();
}

BurstTimer::~BurstTimer()
{
    for(int i=0;i<_slot_times1.size();i++)
    {
        delete _slot_times1.at(i);
    }
    for(int i=0;i<_slot_times2.size();i++)
    {
        delete _slot_times2.at(i);
    }

}

void BurstTimer::set_enabled(bool value)
{
    _enabled = value;
}

void BurstTimer::set_last_timestamp(int cn, uint64_t value)
{
    std::unique_lock<std::mutex> guard(_last_timestamp_mutex);
    _last_timestamp = value;
}

uint64_t BurstTimer::get_last_timestamp(int cn)
{
    std::unique_lock<std::mutex> guard(_last_timestamp_mutex);
    return _last_timestamp;

}

void BurstTimer::set_params(uint64_t samples_per_slot, uint64_t time_per_sample,
                            uint64_t slot_time, uint64_t burst_delay)
{
    _samples_per_slot = samples_per_slot;
    _time_per_sample = time_per_sample;
    _slot_time = slot_time;
    _burst_delay = burst_delay;
}

uint64_t BurstTimer::get_time_delta()
{
    std::unique_lock<std::mutex> guard(_timing_mutex);
    t2 = std::chrono::high_resolution_clock::now();
    return _time_base + std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();
}

void BurstTimer::reset_timer()
{
    std::unique_lock<std::mutex> guard(_timing_mutex);
    _sample_counter = 0;
    _time_base = 0;
    t1 = std::chrono::high_resolution_clock::now();
    //qDebug() << "================= Restarted burst timer =======================";
}

void BurstTimer::set_timer(uint64_t value)
{
    std::unique_lock<std::mutex> guard(_timing_mutex);
    //qDebug() << "================= Set timer: " << value << " ===================";
    _sample_counter = 0;
    _time_base = value;
    t1 = std::chrono::high_resolution_clock::now();
}

void BurstTimer::increment_sample_counter()
{
    std::unique_lock<std::mutex> guard(_timing_mutex);
    _sample_counter++;
}


int BurstTimer::check_time(int cn)
{
    if(!_enabled)
        return 0;
    slot *s;
    std::unique_lock<std::mutex> guard(_slot_mutex);
    if(cn == 1)
    {
        if(_slot_times2.size() < 1)
            return 0;
        s = _slot_times2[0];
    }
    else
    {
        if(_slot_times1.size() < 1)
            return 0;
        s = _slot_times1[0];
    }
    uint64_t sample_time = _time_base + _sample_counter * _time_per_sample;
    if(sample_time >= s->slot_time && s->slot_sample_counter == 0)
    {
        s->slot_sample_counter++;
        return s->slot_no;
    }
    else if(sample_time >= s->slot_time)
    {
        if(s->slot_sample_counter >= (_samples_per_slot - 1))
        {
            if(cn == 1)
            {
                delete _slot_times2[0];
                _slot_times2.removeFirst();
            }
            else
            {
                delete _slot_times1[0];
                _slot_times1.removeFirst();
            }
            //qDebug() << "============= Slots remaining: " << _slot_times.size();
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
    uint64_t elapsed = get_time_delta();
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
    std::unique_lock<std::mutex> guard(_slot_mutex);
    switch(cn)
    {
    case 1:
        _slot_times2.append(s);
        break;
    case 0:
    default:
        _slot_times1.append(s);
    }

    /// send with timestamp in advance with X nanoseconds
    /// this accounts for delay in RF stage
    return nsec - PHY_DELAY;
}



