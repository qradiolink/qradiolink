#include "gr_mmdvm_sink.h"
#include <QDebug>
#include "src/bursttimer.h"
#define RPI
#include "Globals.h"


gr_mmdvm_sink_sptr
make_gr_mmdvm_sink ()
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_sink);
}


gr_mmdvm_sink::gr_mmdvm_sink() :
        gr::block("gr_mmdvm_sink",
                       gr::io_signature::make (1, 1, sizeof (short)),
                       gr::io_signature::make (0, 0, 0))
{
    set_tag_propagation_policy(TPP_ALL_TO_ALL);
}

gr_mmdvm_sink::~gr_mmdvm_sink()
{

}



int gr_mmdvm_sink::general_work(int noutput_items, gr_vector_int &ninput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) output_items;
    short *in = (short*)(input_items[0]);
    ::pthread_mutex_lock(&m_RXlock);
    for(int i = 0;i < noutput_items; i++)
    {
        uint8_t control = MARK_NONE;
        m_rxBuffer.put((uint16_t)in[i], control);
        //burst_timer.check_time(time);
    }
    ::pthread_mutex_unlock(&m_RXlock);

    pmt::pmt_t TIME_TAG = pmt::string_to_symbol("rx_time");
    gr::thread::scoped_lock guard(_mutex);
    std::vector<gr::tag_t> tags;
    get_tags_in_window(tags, 0, 0, noutput_items);
    if (!tags.empty()) {

        //std::sort(tags.begin(), tags.end(), gr::tag_t::offset_compare);
        // Go through the tags
        for (gr::tag_t cTag : tags) {
            //std::cout << "Tag: " << pmt::symbol_to_string(cTag.key) << std::endl;
            // Found tx_time tag
            if (pmt::eq(cTag.key, TIME_TAG)) {
                // Convert time to sample timestamp
                uint64_t secs = pmt::to_uint64(pmt::tuple_ref(cTag.value, 0));
                double fracs = pmt::to_double(pmt::tuple_ref(cTag.value, 1));
                qDebug() << "Secs: " << secs << " Fracs: " << QString::number(fracs, 'f', 20);
                //uint64_t time = secs + uint64_t(fracs * 1000000000.0d);

            }
        }
    }


    consume(0, noutput_items);
    return noutput_items;
}
