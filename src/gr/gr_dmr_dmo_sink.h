// Based on DMRDMORX.cpp code from MMDVM
// Copyright (C) 2009-2017,2020 by Jonathan Naylor G4KLX
// Licensed under GPLv2 or later
// Adapted for GNU Radio by Adrian Musceac YO8RZZ, started March 2025.
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

#ifndef GR_DMR_DMO_SINK_H
#define GR_DMR_DMO_SINK_H

#include <gnuradio/block.h>
#include <gnuradio/io_signature.h>
#include <vector>
#include "src/DMR/constants.h"
#include "src/DMR/dmrframe.h"
#include "src/MMDVM/DMRSlotType2.h"

class gr_dmr_dmo_sink;
typedef std::shared_ptr<gr_dmr_dmo_sink> gr_dmr_dmo_sink_sptr;

gr_dmr_dmo_sink_sptr make_gr_dmr_dmo_sink();

class gr_dmr_dmo_sink : public gr::block
{
public:
    gr_dmr_dmo_sink();
    ~gr_dmr_dmo_sink();
    std::vector<DMRFrame> get_data();
    int general_work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);

private:
    gr::thread::mutex _mutex;
    uint8_t countSyncErrs(uint64_t sync);
    void  reset();
    void writeFrame(uint8_t *frame, uint8_t type);
    std::vector<DMRFrame> _frame_buffer;
    uint32_t    m_bitBuffer[SYMBOL_LENGTH_SAMPLES];
    float       m_buffer[DMO_BUFFER_LENGTH_SAMPLES];
    uint16_t    m_bitPtr;
    uint16_t    m_dataPtr;
    uint16_t    m_syncPtr;
    uint16_t    m_startPtr;
    uint16_t    m_endPtr;
    float       m_maxCorr;
    float       m_centre[4U];
    float       m_threshold[4U];
    uint8_t     m_averagePtr;
    uint8_t     m_syncCount;
    RECV_STATE m_state;
    uint8_t     m_control;
    uint8_t     m_n;
    uint8_t     m_colorCode;

    bool processSample(float sample);
    void correlateSync(bool first);
    void samplesToBits(uint16_t start, uint8_t count, unsigned char* buffer,
                       uint16_t offset, float centre, float threshold);
};


#endif // GR_DMR_DMO_SINK_H
