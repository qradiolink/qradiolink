#include "bursttimer.h"
#include <QDebug>

static const uint64_t BURST_DELAY = 400000; // microseconds

BurstTimer::BurstTimer()
{

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
    slot s;
    s.slot_no = slot_no;
    s.slot_time = slot_time;
    boost::unique_lock<boost::mutex> guard(_timing_mutex);
    _slot_times.append(s);
}

void BurstTimer::pop_slot()
{
    boost::unique_lock<boost::mutex> guard(_timing_mutex);
    _slot_times.removeFirst();
}

int BurstTimer::check_time(uint64_t arrival_time)
{
    boost::unique_lock<boost::mutex> guard(_timing_mutex);
    slot s = _slot_times.at(0);
    if(arrival_time >= s.slot_time && _slot_times.size() > 0)
    {
        _slot_times.removeFirst();
        return s.slot_no;
    }
    return 0;
}

void BurstTimer::allocate_slot(int slot_no)
{
    boost::unique_lock<boost::mutex> guard(_timing_mutex);
    slot s;
    s.slot_no = slot_no;
    s.slot_time = _burst_timer.nsecsElapsed() / 1000 + BURST_DELAY;
    _slot_times.append(s);
}

BurstTimer burst_timer;


