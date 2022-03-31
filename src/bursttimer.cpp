#include "bursttimer.h"
#include <QDebug>

static const uint64_t BURST_DELAY = 300000000; // nanosec

BurstTimer::BurstTimer()
{
    _sample_counter = 0;
}

void BurstTimer::reset_timer()
{
    boost::unique_lock<boost::mutex> guard(_timing_mutex);
    _sample_counter = 0;
    _burst_timer.restart();
    qDebug() << "Restarted burst timer";
}

void BurstTimer::increment_sample_counter()
{
    boost::unique_lock<boost::mutex> guard(_timing_mutex);
    _sample_counter++;
}

void BurstTimer::add_slot(uint8_t slot_no, uint64_t slot_time)
{
    slot *s = new slot;
    s->slot_no = slot_no;
    s->slot_time = slot_time;
    boost::unique_lock<boost::mutex> guard(_slot_mutex);
    _slot_times.append(s);
}

void BurstTimer::pop_slot()
{
    boost::unique_lock<boost::mutex> guard(_slot_mutex);
    if(_slot_times.size() > 0)
        _slot_times.removeFirst();
}

int BurstTimer::check_time()
{
    boost::unique_lock<boost::mutex> guard(_slot_mutex);
    if(_slot_times.size() < 1)
        return 0;
    slot *s = _slot_times[0];
    uint64_t sample_time = _sample_counter * 41667;
    if(sample_time >= s->slot_time && s->slot_sample_counter == 0)
    {
        s->slot_sample_counter++;
        return s->slot_no;
    }
    else if(sample_time >= s->slot_time)
    {
        if(s->slot_sample_counter >= 719)
        {
            delete _slot_times[0];
            _slot_times.removeFirst();
            //qDebug() << "Slots: " << _slot_times.size();
            return 0;
        }
        s->slot_sample_counter++;
    }
    return 0;
}

uint64_t BurstTimer::allocate_slot(int slot_no)
{
    slot *s = new slot;
    s->slot_no = (uint8_t)slot_no;
    uint64_t usec = _burst_timer.nsecsElapsed() + BURST_DELAY;
    s->slot_time = usec;
    s->slot_sample_counter = 0;
    boost::unique_lock<boost::mutex> guard(_slot_mutex);
    _slot_times.append(s);
    return usec;
}

BurstTimer burst_timer;


