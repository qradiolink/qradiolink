#ifndef GR_MMDV_SINK_H
#define GR_MMDV_SINK_H

#include <gnuradio/sync_block.h>
#include <gnuradio/sync_interpolator.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/tags.h>
#include "src/bursttimer.h"


class gr_mmdvm_sink;
typedef boost::shared_ptr<gr_mmdvm_sink> gr_mmdvm_sink_sptr;

gr_mmdvm_sink_sptr make_gr_mmdvm_sink(BurstTimer *burst_timer);

class gr_mmdvm_sink : public gr::block
{
public:
    gr_mmdvm_sink(BurstTimer *burst_timer);
    ~gr_mmdvm_sink();
    int general_work(int noutput_items,
                     gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);


private:
    BurstTimer *_burst_timer;

};


#endif // GR_MMDV_SINK_H