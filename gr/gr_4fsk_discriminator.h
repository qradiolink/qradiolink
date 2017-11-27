#ifndef GR_4FSK_DISCRIMINATOR_H
#define GR_4FSK_DISCRIMINATOR_H

#include <gnuradio/sync_block.h>
#include <gnuradio/sync_interpolator.h>
#include <gnuradio/io_signature.h>
#include <stdio.h>

class gr_4fsk_discriminator;
typedef boost::shared_ptr<gr_4fsk_discriminator> gr_4fsk_discriminator_sptr;

gr_4fsk_discriminator_sptr make_gr_4fsk_discriminator();

class gr_4fsk_discriminator : public gr::sync_block
{
public:
    explicit gr_4fsk_discriminator();

    int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

};

#endif // GR_4FSK_DISCRIMINATOR_H
