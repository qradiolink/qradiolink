#ifndef GR_MMDVM_FLOW_PAD_H
#define GR_MMDVM_FLOW_PAD_H

#include <gnuradio/sync_block.h>
#include <gnuradio/sync_interpolator.h>
#include <gnuradio/io_signature.h>
#include <stdio.h>


class gr_mmdvm_flow_pad;
typedef boost::shared_ptr<gr_mmdvm_flow_pad> gr_mmdvm_flow_pad_sptr;

gr_mmdvm_flow_pad_sptr make_gr_mmdvm_flow_pad();

class gr_mmdvm_flow_pad : public gr::block
{
public:
    explicit gr_mmdvm_flow_pad();

    int general_work(int noutput_items,
                     gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

};

#endif // GR_MMDVM_FLOW_PAD_H
