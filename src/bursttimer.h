#ifndef BURSTTIMER_H
#define BURSTTIMER_H

#include <boost/thread/mutex.hpp>
#include <QElapsedTimer>
#include <QVector>

class BurstTimer
{
public:
    BurstTimer();

    void reset_timer();
    void set_timer(uint64_t value);
    void increment_sample_counter();
    void add_slot(uint8_t slot_no, uint64_t slot_time);
    void pop_slot();
    int check_time();
    uint64_t allocate_slot(int slot_no);

private:
    boost::mutex _timing_mutex;
    boost::mutex _slot_mutex;
    struct slot {
        uint8_t slot_no;
        uint64_t slot_time;
        uint64_t slot_sample_counter;
    };
    uint64_t _sample_counter;
    QElapsedTimer _burst_timer;
    QVector<slot*> _slot_times;
};

extern BurstTimer burst_timer;

#endif // BURSTTIMER_H
