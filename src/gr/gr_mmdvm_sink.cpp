#include "gr_mmdvm_sink.h"
#include <QDebug>
#define RPI
#include "Globals.h"


gr_mmdvm_sink_sptr
make_gr_mmdvm_sink (BurstTimer *burst_timer)
{
    return gnuradio::get_initial_sptr(new gr_mmdvm_sink(burst_timer));
}

static const pmt::pmt_t TIME_TAG = pmt::string_to_symbol("rx_time");

gr_mmdvm_sink::gr_mmdvm_sink(BurstTimer *burst_timer) :
        gr::block("gr_mmdvm_sink",
                       gr::io_signature::make (1, 1, sizeof (short)),
                       gr::io_signature::make (0, 0, 0))
{
    set_tag_propagation_policy(TPP_ALL_TO_ALL);
    _burst_timer = burst_timer;
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
                //qDebug() << "Secs: " << secs << " Fracs: " << QString::number(fracs, 'f', 20);
                uint64_t time = uint64_t((double)secs * 1000000000.0d) + uint64_t(fracs * 1000000000.0d);
                _burst_timer->set_timer(time);

            }
        }
    }

    ::pthread_mutex_lock(&m_RXlock);
    for(int i = 0;i < noutput_items; i++)
    {
        uint8_t control = MARK_NONE;
        int slot_no = 0;

        slot_no = _burst_timer->check_time();

        if(slot_no == 1)
        {
            control = MARK_SLOT1;
            //qDebug() << "RX slot 1";
        }
        if(slot_no == 2)
        {
            control = MARK_SLOT2;
            //qDebug() << "RX slot 2";
        }
        m_rxBuffer.put((uint16_t)in[i], control);
        _burst_timer->increment_sample_counter();

    }
    ::pthread_mutex_unlock(&m_RXlock);


    consume(0, noutput_items);
    return noutput_items;
}
