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
    _last_slot = 0;
    _time_base = 0;
    t1 = std::chrono::high_resolution_clock::now();
}

BurstTimer::~BurstTimer()
{
    for(int i=0;i<_slot_times.size();i++)
    {
        delete _slot_times.at(i);
    }

}

void BurstTimer::set_enabled(bool value)
{
    _enabled = value;
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


int BurstTimer::check_time()
{
    if(!_enabled)
        return 0;
    std::unique_lock<std::mutex> guard(_slot_mutex);
    if(_slot_times.size() < 1)
        return 0;
    slot *s = _slot_times[0];
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
            delete _slot_times[0];
            _slot_times.removeFirst();
            //qDebug() << "============= Slots remaining: " << _slot_times.size();
            return 0;
        }
        s->slot_sample_counter++;
    }
    return 0;
}

uint64_t BurstTimer::allocate_slot(int slot_no)
{
    if(!_enabled)
        return 0L;
    slot *s = new slot;
    s->slot_no = (uint8_t)slot_no;
    uint64_t elapsed = get_time_delta();
    if(elapsed <= _last_slot)
    {
        _last_slot = _last_slot + _slot_time;
    }
    else if(_last_slot == 0)
    {
        _last_slot = elapsed;
    }
    else if((elapsed - _last_slot) >= (10L * _slot_time))
    {
        _last_slot = elapsed;
    }
    else
    {
        _last_slot = _last_slot + _slot_time;
    }
    uint64_t nsec = _last_slot + _burst_delay;
    s->slot_time = nsec;
    s->slot_sample_counter = 0;
    std::unique_lock<std::mutex> guard(_slot_mutex);
    _slot_times.append(s);
    /// send with timestamp in advance with X nanoseconds
    /// this accounts for delay in RF stage
    return nsec - PHY_DELAY;
}



