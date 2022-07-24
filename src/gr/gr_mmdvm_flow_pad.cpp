#include "gr_mmdvm_flow_pad.h"

gr_mmdvm_flow_pad_sptr
make_gr_mmdvm_flow_pad ()
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_flow_pad);
}

gr_mmdvm_flow_pad::gr_mmdvm_flow_pad() :
    gr::block("gr_mmdvm_flow_pad",
                   gr::io_signature::make (2, 2, sizeof (short)),
                   gr::io_signature::make (2, 2, sizeof (short)))
{
}

int gr_mmdvm_flow_pad::general_work(int noutput_items, gr_vector_int &ninput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    short *in1 = (short*)(input_items[0]);
    short *in2 = (short*)(input_items[1]);

    short *out1 = (short*)(output_items[0]);
    short *out2 = (short*)(output_items[1]);
    std::cout << "1: " << ninput_items[0] << " 2: " << ninput_items[1] << std::endl;

    if((ninput_items[0] == 0) && (ninput_items[1] == 0))
    {
        for(int i = 0;i< noutput_items;i++)
        {
            out1[i] = 0;
            out2[i] = 0;
        }
        consume(0, 0);
        consume(1, 0);
        return noutput_items;
    }
    else if((ninput_items[0] > 0) && (ninput_items[1] > 0))
    {
        for(int i = 0;i< noutput_items;i++)
        {
            out1[i] = in1[i];
            out2[i] = in2[i];
        }
        consume(0, noutput_items);
        consume(1, noutput_items);
        return noutput_items;
    }
    else if((ninput_items[0] > 0) && (ninput_items[1] == 0))
    {
        for(int i = 0;i< noutput_items;i++)
        {
            out1[i] = in1[i];
            out2[i] = 0;
        }
        consume(0, noutput_items);
        consume(1, 0);
        return noutput_items;
    }
    else if((ninput_items[0] == 0) && (ninput_items[1] > 0))
    {
        for(int i = 0;i< noutput_items;i++)
        {
            out1[i] = 0;
            out2[i] = in2[i];
        }
        consume(0, 0);
        consume(1, noutput_items);
        return noutput_items;
    }

}

