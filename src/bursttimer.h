#ifndef BURSTTIMER_H
#define BURSTTIMER_H

#include <mutex>
#include <chrono>
#include <QVector>

class BurstTimer
{
public:
    BurstTimer();
    ~BurstTimer();

    void reset_timer();
    uint64_t get_time_delta();
    void set_timer(uint64_t value);
    void increment_sample_counter();
    int check_time();
    uint64_t allocate_slot(int slot_no);

private:
    std::mutex _timing_mutex;
    std::mutex _slot_mutex;
    struct slot {
        uint8_t slot_no;
        uint64_t slot_time;
        uint64_t slot_sample_counter;
    };
    uint64_t _sample_counter;
    uint64_t _last_slot;
    std::chrono::high_resolution_clock::time_point t1;
    std::chrono::high_resolution_clock::time_point t2;
    QVector<slot*> _slot_times;
    uint64_t _time_base;
};


#endif // BURSTTIMER_H
