// Written by Adrian Musceac YO8RZZ , started October 2024.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


#include "gr_dmr_sink.h"

#include <iostream>

static const pmt::pmt_t TIME_TAG = pmt::string_to_symbol("rx_time");

gr_dmr_sink_sptr
make_gr_dmr_sink (DMRTiming *dmrtiming)
{
    return gnuradio::get_initial_sptr(new gr_dmr_sink(dmrtiming));
}

gr_dmr_sink::gr_dmr_sink(DMRTiming *dmrtiming) :
        gr::sync_block("gr_dmr_sink",
                       gr::io_signature::make (1, 1, sizeof (unsigned char)),
                       gr::io_signature::make (0, 0, 0))
{
    _shift_register[0] = 0;
    _shift_register[1] = 0;
    _bits_to_receive[0] = 0;
    _bits_to_receive[1] = 0;
    _frames_to_receive[0] = 0;
    _frames_to_receive[1] = 0;
    _state[0] = RECV_NONE;
    _state[1] = RECV_NONE;
    _downlink[0] = true;
    _downlink[1] = true;
    _next_slot = 0;
    _symbol_bits = 0;
    _slot_sample_counter[0] = 0;
    _slot_sample_counter[1] = 0;
    _dmr_timing = dmrtiming;
}

gr_dmr_sink::~gr_dmr_sink()
{
}

void gr_dmr_sink::flush()
{
    gr::thread::scoped_lock guard(_mutex);
    _bit_buffer[0].clear();
    _bit_buffer[1].clear();
}

std::vector<DMRFrame> gr_dmr_sink::get_data()
{
    gr::thread::scoped_lock guard(_mutex);
    std::vector<DMRFrame> data;
    if(_frame_buffer.size() < 1)
    {
        return data;
    }
    for(uint32_t i = 0;i<_frame_buffer.size();i++)
    {
        data.push_back(_frame_buffer.at(i));
    }
    _frame_buffer.clear();

    return data;
}

bool gr_dmr_sink::processBits(uint8_t bit, uint8_t ts_index)
{
    uint8_t slot_no = 0;
    uint8_t next_slot = nextSlot(ts_index);
    bool sync = false;
    if(_state[ts_index] != RECV_NONE)
    {
        if((_state[ts_index] == RECV_DATA) || (_state[ts_index] == RECV_VOICE_SYNC))
        {
            if (_bits_to_receive[ts_index] > 0)
            {
                _bits_to_receive[ts_index]--;
                if(_bits_to_receive[ts_index] == 0)
                {
                    uint8_t frame_type = (_state[ts_index] == RECV_DATA) ? DMRFrameType::DMRFrameTypeData : DMRFrameType::DMRFrameTypeVoiceSync;
                    DMRFrame frame(_bit_buffer[ts_index], frame_type);
                    if(_state[ts_index] == RECV_VOICE_SYNC)
                    {
                        //std::cout << "Audio Sync Frame: " << _bit_buffer.size() << std::endl;
                        _state[ts_index] = RECV_VOICE;
                    }
                    if(_state[ts_index] == RECV_DATA)
                    {
                        _state[ts_index] = RECV_NONE;
                    }
                    bool cach_decoded = frame.setDownlink(_downlink[ts_index]);
                    slot_no = frame.getSlotNo();
                    if(_downlink[ts_index] && cach_decoded && (slot_no > 0) && (slot_no < 3))
                        _dmr_timing->set_slot_times(slot_no);
                    _frame_buffer.push_back(frame);
                    _bit_buffer[ts_index].clear();
                    _next_slot = next_slot;
                }
            }
        }
        else if((_state[ts_index] == RECV_VOICE) && (_frames_to_receive[ts_index] > 0))
        {
            if(_bit_buffer[ts_index].size() >= CACH_LENGTH_BITS + FRAME_LENGTH_BITS)
            {
                //std::cout << "Voice Frame: " << _bit_buffer.size() << std::endl;
                DMRFrame frame(_bit_buffer[ts_index], DMRFrameType::DMRFrameTypeVoice);
                frame.setFN(6 - _frames_to_receive[ts_index]);
                bool cach_decoded = frame.setDownlink(_downlink[ts_index]);
                slot_no = frame.getSlotNo();
                if(_downlink[ts_index] && cach_decoded && (slot_no > 0) && (slot_no < 3))
                    _dmr_timing->set_slot_times(slot_no);
                _frame_buffer.push_back(frame);
                _bit_buffer[ts_index].clear();
                _frames_to_receive[ts_index]--;
                if(_frames_to_receive[ts_index] == 0)
                {
                    _state[ts_index] = RECV_NONE;
                }
                _next_slot = next_slot;
            }
        }
    }
    else if(_state[ts_index] == RECV_NONE)
    {
        sync = findSync(bit, ts_index);
    }
    return sync;
}

int gr_dmr_sink::work(int noutput_items,
       gr_vector_const_void_star &input_items,
       gr_vector_void_star &output_items)
{

    (void) output_items;
    if(noutput_items < 1)
    {
        return noutput_items;
    }
    gr::thread::scoped_lock guard(_mutex);
    std::vector<gr::tag_t> tags;
    uint64_t nitems = nitems_read(0);

    get_tags_in_window(tags, 0, 0, noutput_items, TIME_TAG);
    if (!tags.empty()) {

        std::sort(tags.begin(), tags.end(), gr::tag_t::offset_compare);
    }



    for(int i=0;i<2;i++)
    {
        if(_bit_buffer[i].size() >= (3 * (CACH_LENGTH_BITS + FRAME_LENGTH_BITS)))
        {
            _bit_buffer[i].erase(_bit_buffer[i].begin(), _bit_buffer[i].begin() + CACH_LENGTH_BITS + FRAME_LENGTH_BITS);
        }
    }
    unsigned char *in = (unsigned char*)(input_items[0]);
    for(int i=0;i < noutput_items;i++)
    {
        bool time_base_received = false;
        if(_slot_sample_counter[_next_slot] > 0)
        {
            _slot_sample_counter[_next_slot]++;
        }
        for (gr::tag_t tag : tags)
        {
            if(tag.offset == nitems + (uint64_t)i)
            {
                uint64_t secs = pmt::to_uint64(pmt::tuple_ref(tag.value, 0));
                double fracs = pmt::to_double(pmt::tuple_ref(tag.value, 1));
                // nanoseconds
                uint64_t time = uint64_t(llround(double(secs * 1000000000L) + (fracs * 1000000000.0d)));
                _dmr_timing->set_timer(time);
                time_base_received = true;
                break;
            }
        }
        nextBit();
        if(!time_base_received && (_symbol_bits == 0))
            _dmr_timing->increment_sample_counter(SYMBOL_LENGTH_SAMPLES);
        uint8_t bit = (uint8_t) in[i];
        _bit_buffer[_next_slot].push_back(bit);
        processBits(bit, _next_slot);
    }
    return noutput_items;
}

bool gr_dmr_sink::findSync(uint8_t bit, uint8_t ts_index)
{
    _shift_register[ts_index] = (_shift_register[ts_index] << 1) | (bit & 0x1);
    uint64_t temp = _shift_register[ts_index] & SYNC_BITS_MASK;
    uint64_t sync;
    uint8_t errs;
    sync = temp ^ MS_DATA_SYNC_BITS;
    errs = countSyncErrs(sync);
    bool ms_data_sync = errs < 1;
    sync = temp ^ MS_VOICE_SYNC_BITS;
    errs = countSyncErrs(sync);
    bool ms_voice_sync = errs < 1;
    sync = temp ^ BS_DATA_SYNC_BITS;
    errs = countSyncErrs(sync);
    bool bs_data_sync = errs < 1;
    sync = temp ^ BS_VOICE_SYNC_BITS;
    errs = countSyncErrs(sync);
    bool bs_voice_sync = errs < 1;

    if(bs_data_sync || ms_data_sync)
    {
        _state[ts_index] = RECV_DATA;
    }
    else if(bs_voice_sync || ms_voice_sync)
    {
        _state[ts_index] = RECV_VOICE_SYNC;
    }
    if(bs_data_sync || bs_voice_sync)
    {
        _downlink[ts_index] = true;
    }
    else if(ms_data_sync || ms_voice_sync)
    {
        _downlink[ts_index] = false;
    }
    if(_state[ts_index] != RECV_NONE)
    {
        if(_bit_buffer[ts_index].size() < DATA_AND_SYNC_BITS)
        {
            _state[ts_index] = RECV_NONE;
            _bits_to_receive[ts_index] = 0;
            _frames_to_receive[ts_index] = 0;
            _bit_buffer[ts_index].clear();
            return false;
        }
        else if(_bit_buffer[ts_index].size() > DATA_AND_SYNC_BITS)
        {
            uint32_t bits_to_remove = _bit_buffer[ts_index].size() - DATA_AND_SYNC_BITS;
            _bit_buffer[ts_index].erase(_bit_buffer[ts_index].begin(), _bit_buffer[ts_index].begin() + bits_to_remove);
        }
        _bits_to_receive[ts_index] = PAYLOAD_LENGTH_BITS;
        if(_state[ts_index] == RECV_VOICE_SYNC)
        {
            _frames_to_receive[ts_index] = 5;
        }

        return true;
    }
    return false;
}

uint8_t gr_dmr_sink::countSyncErrs(uint64_t sync)
{
    uint8_t errs = 0;
    while(sync)
    {
        errs += sync & 1;
        sync >>= 1;
    }
    return errs;
}

uint8_t gr_dmr_sink::nextSlot(uint8_t ts_index)
{
    if(ts_index == 0)
        return 1;
    return 0;
}

void gr_dmr_sink::nextBit()
{
    _symbol_bits += 1;
    if(_symbol_bits > 1)
        _symbol_bits = 0;
}
