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

#include "dmrtiming.h"
#include <QDebug>


DMRTiming::DMRTiming(const Settings *settings, uint64_t samples_per_slot, uint64_t time_per_sample,
                     uint64_t slot_time, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _samples_per_slot = samples_per_slot;
    _time_per_sample = time_per_sample;
    _slot_time = slot_time;
    _slot_times[0] = 0L;
    _slot_times[1] = 0L;
    _sample_counter = 0L;
    _time_base = 0L;
    _next_tx_time = 0L;
    _tx = false;
    _first = false;
    t1 = std::chrono::high_resolution_clock::now();
}


void DMRTiming::set_params(uint64_t samples_per_slot, uint64_t time_per_sample,
                            uint64_t slot_time)
{
    _samples_per_slot = samples_per_slot;
    _time_per_sample = time_per_sample;
    _slot_time = slot_time;
}

void DMRTiming::reset_timer()
{
    std::scoped_lock<std::mutex> guard(_timing_mutex);
    _sample_counter = 0;
    _time_base = 0;
}

void DMRTiming::set_timer(uint64_t value)
{
    std::scoped_lock<std::mutex> guard(_timing_mutex);
    //qDebug() << "================= Set timer: " << value << " ===================";
    _sample_counter = 0;
    _time_base = value;
}

void DMRTiming::increment_sample_counter(uint64_t no_of_samples)
{
    std::scoped_lock<std::mutex> guard(_timing_mutex);
    _sample_counter += no_of_samples;
}

uint64_t DMRTiming::get_sample_counter()
{
    std::scoped_lock<std::mutex> guard(_timing_mutex);
    return _time_base + _sample_counter * _time_per_sample;
}

void DMRTiming::set_slot_times(uint8_t sn)
{
    sn = sn - 1;
    std::scoped_lock<std::mutex> guard(_slot_mutex[sn]);
    _slot_times[sn] = get_sample_counter(); // acquire the time base in sample number for this timeslot
    t1 = std::chrono::high_resolution_clock::now();
    if(!_tx && (_settings->dmr_mode != DMR_MODE::DMR_MODE_DMO))
        emit timing_ready(sn + 1);
}

bool DMRTiming::timing_recent(uint8_t sn)
{
    (void) sn;
    // Unfortunately, gnuradio operates with large buffers, so timing updates can happen
    // at long intervals with multiples
    // time bases for the slot received at once in large sample packets
    // Use a large time delta to check if timing information was recently acquired
    t2 = std::chrono::high_resolution_clock::now();
    uint64_t last_slot_time_update =
            (uint64_t) std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();
    if(last_slot_time_update < 12 * SLOT_TIME)
        return true;
    return false;
}

uint64_t DMRTiming::get_slot_times(uint8_t sn)
{
    std::scoped_lock<std::mutex> guard_tx_state(_tx_mutex);
    if(!_tx || (sn == 0))
    {
        // if the frame has no slot info, reset timing info
        _tx = false;
        _first = false;
        return 0L;
    }
    sn = sn - 1;
    std::scoped_lock<std::mutex> guard(_slot_mutex[sn]);
    if(_first)
    {
        // acquire current slot timing
        _next_tx_time = _slot_times[sn] +
                3 * SLOT_TIME +  // normally should be 2 slot periods, but since filter delay can't be controlled
                                 // properly in gnuradio, compensate for it here (but is buggy)
                ((CACH_LENGTH_BITS / 2) * SYMBOL_LENGTH_SAMPLES) * TIME_PER_SAMPLE +
                _settings->dmr_timing_correction * TIME_PER_SAMPLE; // timing correction is in samples
        _first = false;
    }
    else
    {
        // once time base was established, future bursts can occur at regular intervals
        _next_tx_time = _next_tx_time + 2 * SLOT_TIME;
    }
    return _next_tx_time;
}

void DMRTiming::set_tx_time(bool value)
{
    std::scoped_lock<std::mutex> guard(_tx_mutex);
    _first = value;
    _tx = value;
}

bool DMRTiming::get_tx_time()
{
    std::scoped_lock<std::mutex> guard(_tx_mutex);
    bool tx = _tx;
    return tx;
}

