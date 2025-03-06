#include "gr_dmr_source.h"
#include <QString>

#include <gnuradio/pdu.h>

static const pmt::pmt_t TIME_TAG = pmt::string_to_symbol("tx_time");
static const pmt::pmt_t LENGTH_TAG = pmt::string_to_symbol("burst_length");
static const pmt::pmt_t ZERO_TAG = pmt::string_to_symbol("zero_samples");

static const uint16_t DMR_ZERO_TX_LENGTH_BYTES = FRAME_LENGTH_BYTES + 2 * CACH_LENGTH_BYTES;
static const uint64_t DMR_ZERO_TX_LENGTH_SAMPLES = DMR_ZERO_TX_LENGTH_BYTES * 4 * SYMBOL_LENGTH_SAMPLES;

const pmt::pmt_t port_id = pmt::intern("dmr_source_msg_port");

gr_dmr_source_sptr
make_gr_dmr_source (DMRTiming *dmrtiming)
{
    return gnuradio::get_initial_sptr(new gr_dmr_source(dmrtiming));
}

gr_dmr_source::gr_dmr_source(DMRTiming *dmrtiming) :
        gr::sync_block("gr_dmr_source",
                       gr::io_signature::make (0, 0, 0),
                       gr::io_signature::make (1, 1, sizeof (unsigned char)))
{
    _dmr_timing = dmrtiming;
    _init_counter = 0;
    message_port_register_in(port_id);
    set_msg_handler(port_id,
       [this](const pmt::pmt_t& msg) { message_handler_function(msg); });
    //set_min_noutput_items(DMR_FRAME_LENGTH_BYTES + DMR_ZERO_TX_LENGTH_BYTES);
}

gr_dmr_source::~gr_dmr_source()
{
}


void gr_dmr_source::flush()
{
    gr::thread::scoped_lock guard(_mutex);
    _frame_buffer.clear();
}

int gr_dmr_source::set_data(std::vector<DMRFrame> &frames)
{

    gr::thread::scoped_lock guard(_mutex);
    for(uint i=0;i<frames.size();i++)
    {
        std::vector<uint8_t> bytes = frames.at(i).toByteVector();
        std::vector<uint8_t> zeros(DMR_ZERO_TX_LENGTH_BYTES, 0);
        bytes.insert(bytes.end(), zeros.begin(), zeros.end());
        _frame_buffer.push_back(bytes);
        _slot_numbers.push_back(frames.at(i).getSlotNo());
    }

    frames.clear();
    pmt::pmt_t msg;
    this->_post(port_id, msg);
    return 0;
}

void gr_dmr_source::message_handler_function(const pmt::pmt_t &msg)
{
    (void) msg;
    if(this->nmsgs(port_id) > 0)
        delete_head_nowait(port_id);
}

int gr_dmr_source::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{
    (void) input_items;
    gr::thread::scoped_lock guard(_mutex);
    int frames_remaining = _frame_buffer.size();
    /*
    if(!_dmr_timing->get_tx())
    {
        guard.unlock();
        struct timespec time_to_sleep = {0, 5000000L };
        nanosleep(&time_to_sleep, NULL);
        return 0;
    }
    */
    if(frames_remaining < 1)
    {
        return 0;
    }
    std::vector<uint8_t> current_frame = _frame_buffer.at(0);

    unsigned char *out = (unsigned char*)(output_items[0]);
    unsigned int n = std::min((unsigned int)current_frame.size(),
                                  (unsigned int)noutput_items);
    for(unsigned int i=0;i < n;i++)
    {
        out[i] = current_frame.at(0);
        uint32_t left = current_frame.size();
        if(left == (FRAME_LENGTH_BYTES + DMR_ZERO_TX_LENGTH_BYTES))
        {
            uint8_t slot_no = _slot_numbers.at(0);
            uint64_t time = _dmr_timing->get_slot_times(slot_no);
            if(time > 0L)
            {
                add_time_tag(time, i, 0);
            }
        }
        else if(left == DMR_ZERO_TX_LENGTH_BYTES)
        {
            add_zero_tag(i, DMR_ZERO_TX_LENGTH_SAMPLES, 0);
        }
        current_frame.erase(current_frame.begin());
    }

    if(current_frame.size() < 1)
    {
        _frame_buffer.erase(_frame_buffer.begin());
        _slot_numbers.erase(_slot_numbers.begin());
    }
    return n;
}

// Add tx_time tag to stream
void gr_dmr_source::add_time_tag(uint64_t nsec, int offset, int which)
{
    uint64_t intpart = nsec / 1000000000L;
    double fracpart = ((double)nsec / 1000000000.0d) - (double)intpart;

    const pmt::pmt_t t_val = pmt::make_tuple(pmt::from_uint64(intpart), pmt::from_double(fracpart));
    this->add_item_tag(which, nitems_written(which) + (uint64_t)offset, TIME_TAG, t_val);
    /// length tag doesn't seem to be necessary
    //long long value = llround((double)(FRAME_LENGTH_BYTES * 4 * SYMBOL_LENGTH_SAMPLES * 125) / 3.0d);
    //const pmt::pmt_t b_val = pmt::from_long(value);
    //this->add_item_tag(0, nitems_written(0) + offset, LENGTH_TAG, b_val);

}

// Add zero samples tag to stream
void gr_dmr_source::add_zero_tag(int offset, uint64_t num_samples, int which)
{
    const pmt::pmt_t t_val = pmt::from_uint64((uint64_t)num_samples);
    uint64_t start = nitems_written(which) + (uint64_t)offset;
    //std::cout << "Zero tag: " << num_samples << " Pos: " << nitems_written(which) + (uint64_t)offset << std::endl;
    this->add_item_tag(which, start, ZERO_TAG, t_val);
}
