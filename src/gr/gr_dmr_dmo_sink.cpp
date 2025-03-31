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

#include "gr_dmr_dmo_sink.h"

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])

gr_dmr_dmo_sink_sptr make_gr_dmr_dmo_sink()
{
    return gnuradio::get_initial_sptr(new gr_dmr_dmo_sink());
}

gr_dmr_dmo_sink::gr_dmr_dmo_sink() :
        gr::block("gr_dmr_dmo_sink",
                       gr::io_signature::make (1, 1, sizeof (float)),
                       gr::io_signature::make (0, 0, 0))
{
    m_bitPtr = 0U;
    m_dataPtr = 0U;
    m_syncPtr = 0U;
    m_startPtr = 0U;
    m_endPtr = NOENDPTR;
    m_maxCorr = 0.0f;
    m_averagePtr = 0U;
    m_syncCount = 0U;
    m_state = RECV_NONE;
    m_n = 0U;
    m_colorCode = 0U;
}

gr_dmr_dmo_sink::~gr_dmr_dmo_sink()
{
}

void gr_dmr_dmo_sink::reset()
{
    m_syncPtr   = 0U;
    m_maxCorr   = 0;
    m_syncCount = 0U;
    m_state     = RECV_NONE;
    m_startPtr  = 0U;
    m_endPtr    = NOENDPTR;
    m_colorCode = 0U;
    m_n = 0U;
}

int gr_dmr_dmo_sink::general_work(int noutput_items,
                                  gr_vector_int& ninput_items,
                                  gr_vector_const_void_star& input_items,
                                  gr_vector_void_star& output_items)
{
    (void) noutput_items;
    (void) output_items;
    gr::thread::scoped_lock guard(_mutex);
    float *in = (float*)(input_items[0]);
    int ninput = ninput_items[0];
    for(int i=0;i<ninput;i++)
    {
        processSample(in[i]);
    }
    consume_each(ninput);
    return WORK_CALLED_PRODUCE;
}

bool gr_dmr_dmo_sink::processSample(float sample)
{
  m_buffer[m_dataPtr] = sample;

  m_bitBuffer[m_bitPtr] <<= 1;
  if (sample > 0.0f)
    m_bitBuffer[m_bitPtr] |= 0x01U;

  if (m_state == RECV_NONE) {
    correlateSync(true);
  } else {

    uint16_t min  = m_syncPtr + DMO_BUFFER_LENGTH_SAMPLES - 1U;
    uint16_t max  = m_syncPtr + 1U;

    if (min >= DMO_BUFFER_LENGTH_SAMPLES)
      min -= DMO_BUFFER_LENGTH_SAMPLES;
    if (max >= DMO_BUFFER_LENGTH_SAMPLES)
      max -= DMO_BUFFER_LENGTH_SAMPLES;

    if (min < max) {
      if (m_dataPtr >= min && m_dataPtr <= max)
        correlateSync(false);
    } else {
      if (m_dataPtr >= min || m_dataPtr <= max)
        correlateSync(false);
    }
  }

  if (m_dataPtr == m_endPtr) {
    // Find the average centre and threshold values
    float centre    = (m_centre[0U]    + m_centre[1U]    + m_centre[2U]    + m_centre[3U])    / 4.0f;
    float threshold = (m_threshold[0U] + m_threshold[1U] + m_threshold[2U] + m_threshold[3U]) / 4.0f;

    uint8_t frame[DMR_FRAME_LENGTH_BYTES];

    uint16_t ptr = m_endPtr + DMO_BUFFER_LENGTH_SAMPLES - DMR_FRAME_LENGTH_SAMPLES + DMR_RADIO_SYMBOL_LENGTH + 1U;
    if (ptr >= DMO_BUFFER_LENGTH_SAMPLES)
      ptr -= DMO_BUFFER_LENGTH_SAMPLES;

    samplesToBits(ptr, DMR_FRAME_LENGTH_SYMBOLS, frame, 0U, centre, threshold);

    if (m_control == CONTROL_DATA) {
        // Data sync
        uint8_t colorCode;
        uint8_t dataType;
        CDMRSlotType2 slotType;
        slotType.decode(frame, colorCode, dataType);
        m_colorCode = colorCode;
        m_syncCount = 0U;
        m_n         = 0U;

        switch (dataType) {
          case DT_DATA_HEADER:
            m_state = RECV_DATA;
            writeFrame(frame, DMRFrameType::DMRFrameTypeData);
            break;
          case DT_RATE_12_DATA:
          case DT_RATE_34_DATA:
          case DT_RATE_1_DATA:
            if (m_state == RECV_DATA) {
                writeFrame(frame, DMRFrameType::DMRFrameTypeData);
            }
            break;
          case DT_VOICE_LC_HEADER:
            m_state = RECV_VOICE;
            writeFrame(frame, DMRFrameType::DMRFrameTypeData);
            break;
          case DT_VOICE_PI_HEADER:
            m_state = RECV_VOICE;
            writeFrame(frame, DMRFrameType::DMRFrameTypeData);
            break;
          case DT_TERMINATOR_WITH_LC:
            if (m_state == RECV_VOICE) {
                writeFrame(frame, DMRFrameType::DMRFrameTypeData);
                reset();
            }
            break;
          default:    // DT_CSBK
            writeFrame(frame, DMRFrameType::DMRFrameTypeData);
            reset();
            break;
        }
    } else if (m_control == CONTROL_VOICE) {
      // Voice sync
        m_state     = RECV_VOICE;
        m_syncCount = 0U;
        m_n         = 0U;
        writeFrame(frame, DMRFrameType::DMRFrameTypeVoiceSync);
    } else {
      if (m_state != RECV_NONE) {
        m_syncCount++;
        if (m_syncCount >= MAX_SYNC_LOST_FRAMES) {
            reset();
        }
      }

      if (m_state == RECV_VOICE) {
        if (m_n >= 5U) {
          m_n = 0U;
        } else {
          ++m_n;
        }
        writeFrame(frame, DMRFrameType::DMRFrameTypeVoice);
      } else if (m_state == RECV_DATA) {
        writeFrame(frame, DMRFrameType::DMRFrameTypeData);
      }
    }

    // End of this slot, reset some items for the next slot.
    m_maxCorr = 0;
    m_control = CONTROL_NONE;
  }

  m_dataPtr++;
  if (m_dataPtr >= DMO_BUFFER_LENGTH_SAMPLES)
    m_dataPtr = 0U;

  m_bitPtr++;
  if (m_bitPtr >= DMR_RADIO_SYMBOL_LENGTH)
    m_bitPtr = 0U;

  return m_state != RECV_NONE;
}

void gr_dmr_dmo_sink::correlateSync(bool first)
{
    uint8_t data_errs = countSyncErrs((m_bitBuffer[m_bitPtr] & DMR_SYNC_SYMBOLS_MASK) ^ DMR_MS_DATA_SYNC_SYMBOLS);
    uint8_t voice_errs = countSyncErrs((m_bitBuffer[m_bitPtr] & DMR_SYNC_SYMBOLS_MASK) ^ DMR_MS_VOICE_SYNC_SYMBOLS);

    bool data  = (data_errs <= MAX_SYNC_SYMBOLS_ERRS);
    bool voice = (voice_errs <= MAX_SYNC_SYMBOLS_ERRS);

    if (data || voice) {
        uint16_t ptr = m_dataPtr + DMO_BUFFER_LENGTH_SAMPLES - DMR_SYNC_LENGTH_SAMPLES + DMR_RADIO_SYMBOL_LENGTH;
        if (ptr >= DMO_BUFFER_LENGTH_SAMPLES)
          ptr -= DMO_BUFFER_LENGTH_SAMPLES;

        float corr = 0.0f;
        float min =  100.0f;
        float max = -100.0f;

        for (uint8_t i = 0U; i < DMR_SYNC_LENGTH_SYMBOLS; i++) {
          float val = m_buffer[ptr];

          if (val > max)
            max = val;
          if (val < min)
            min = val;

          int8_t corrVal;
          if (data)
            corrVal = DMR_MS_DATA_SYNC_SYMBOLS_VALUES[i];
          else
            corrVal = DMR_MS_VOICE_SYNC_SYMBOLS_VALUES[i];

          corr += float(corrVal) * val;

          ptr += DMR_RADIO_SYMBOL_LENGTH;
          if (ptr >= DMO_BUFFER_LENGTH_SAMPLES)
            ptr -= DMO_BUFFER_LENGTH_SAMPLES;
        }
        if (corr > m_maxCorr) {
            float centre = (max + min) / 2.0f;

            float threshold = (max - centre) / 2.0f;

            uint8_t sync[DMR_SYNC_BYTES_LENGTH];
            uint16_t ptr = m_dataPtr + DMO_BUFFER_LENGTH_SAMPLES - DMR_SYNC_LENGTH_SAMPLES + DMR_RADIO_SYMBOL_LENGTH;
            if (ptr >= DMO_BUFFER_LENGTH_SAMPLES)
            ptr -= DMO_BUFFER_LENGTH_SAMPLES;

            samplesToBits(ptr, DMR_SYNC_LENGTH_SYMBOLS, sync, 4U, centre, threshold);

            if (data) {
                uint8_t errs = 0U;
                for (uint8_t i = 0U; i < DMR_SYNC_BYTES_LENGTH; i++)
                  errs += countSyncErrs((sync[i] & DMR_SYNC_BYTES_MASK[i]) ^ DMR_MS_DATA_SYNC_BYTES[i]);

                if (errs <= MAX_SYNC_BYTES_ERRS) {
                  if (first) {
                    m_threshold[0U] = m_threshold[1U] = m_threshold[2U] = m_threshold[3U] = threshold;
                    m_centre[0U]    = m_centre[1U]    = m_centre[2U]    = m_centre[3U]    = centre;
                    m_averagePtr    = 0U;
                  } else {
                    m_threshold[m_averagePtr] = threshold;
                    m_centre[m_averagePtr]    = centre;

                    m_averagePtr++;
                    if (m_averagePtr >= 4U)
                      m_averagePtr = 0U;
                  }

                  m_maxCorr  = corr;
                  m_control  = CONTROL_DATA;
                  m_syncPtr  = m_dataPtr;

                  m_startPtr = m_dataPtr + DMO_BUFFER_LENGTH_SAMPLES - DMR_SLOT_TYPE_LENGTH_SAMPLES / 2U - DMR_INFO_LENGTH_SAMPLES / 2U - DMR_SYNC_LENGTH_SAMPLES;
                  if (m_startPtr >= DMO_BUFFER_LENGTH_SAMPLES)
                    m_startPtr -= DMO_BUFFER_LENGTH_SAMPLES;

                  m_endPtr = m_dataPtr + DMR_SLOT_TYPE_LENGTH_SAMPLES / 2U + DMR_INFO_LENGTH_SAMPLES / 2U - 1U;
                  if (m_endPtr >= DMO_BUFFER_LENGTH_SAMPLES)
                    m_endPtr -= DMO_BUFFER_LENGTH_SAMPLES;
                }
            }
            else
            {  // if (voice1 || voice2)
                uint8_t errs = 0U;
                for (uint8_t i = 0U; i < DMR_SYNC_BYTES_LENGTH; i++)
                  errs += countSyncErrs((sync[i] & DMR_SYNC_BYTES_MASK[i]) ^ DMR_MS_VOICE_SYNC_BYTES[i]);

                if (errs <= MAX_SYNC_BYTES_ERRS) {
                  if (first) {
                    m_threshold[0U] = m_threshold[1U] = m_threshold[2U] = m_threshold[3U] = threshold;
                    m_centre[0U]    = m_centre[1U]    = m_centre[2U]    = m_centre[3U]    = centre;
                    m_averagePtr    = 0U;
                  } else {
                    m_threshold[m_averagePtr] = threshold;
                    m_centre[m_averagePtr]    = centre;

                    m_averagePtr++;
                    if (m_averagePtr >= 4U)
                      m_averagePtr = 0U;
                  }

                  m_maxCorr  = corr;
                  m_control  = CONTROL_VOICE;
                  m_syncPtr  = m_dataPtr;

                  m_startPtr = m_dataPtr + DMO_BUFFER_LENGTH_SAMPLES - DMR_SLOT_TYPE_LENGTH_SAMPLES / 2U - DMR_INFO_LENGTH_SAMPLES / 2U - DMR_SYNC_LENGTH_SAMPLES;
                  if (m_startPtr >= DMO_BUFFER_LENGTH_SAMPLES)
                    m_startPtr -= DMO_BUFFER_LENGTH_SAMPLES;

                  m_endPtr   = m_dataPtr + DMR_SLOT_TYPE_LENGTH_SAMPLES / 2U + DMR_INFO_LENGTH_SAMPLES / 2U - 1U;
                  if (m_endPtr >= DMO_BUFFER_LENGTH_SAMPLES)
                    m_endPtr -= DMO_BUFFER_LENGTH_SAMPLES;
                }
            }
        }
    }
}

void gr_dmr_dmo_sink::samplesToBits(uint16_t start, uint8_t count, uint8_t* buffer, uint16_t offset,
                                    float centre, float threshold)
{
    for (uint8_t i = 0U; i < count; i++) {
        float sample = m_buffer[start] - centre;

        if (sample < -threshold) {
            WRITE_BIT1(buffer, offset, true);
            offset++;
            WRITE_BIT1(buffer, offset, true);
            offset++;
        } else if (sample < 0.0f) {
            WRITE_BIT1(buffer, offset, true);
            offset++;
            WRITE_BIT1(buffer, offset, false);
            offset++;
        } else if (sample < threshold) {
            WRITE_BIT1(buffer, offset, false);
            offset++;
            WRITE_BIT1(buffer, offset, false);
            offset++;
        } else {
            WRITE_BIT1(buffer, offset, false);
            offset++;
            WRITE_BIT1(buffer, offset, true);
            offset++;
        }

        start += DMR_RADIO_SYMBOL_LENGTH;
        if (start >= DMO_BUFFER_LENGTH_SAMPLES)
          start -= DMO_BUFFER_LENGTH_SAMPLES;
    }
}


uint8_t gr_dmr_dmo_sink::countSyncErrs(uint64_t sync)
{
    uint8_t errs = 0;
    while(sync)
    {
        errs += sync & 1;
        sync >>= 1;
    }
    return errs;
}

void gr_dmr_dmo_sink::writeFrame(uint8_t *frame, uint8_t type)
{
    DMRFrame dmr_frame(frame, type);
    dmr_frame.setDownlink(false);
    dmr_frame.setFN(m_n);
    dmr_frame.setSlotNo(2);
    dmr_frame.setColorCode(m_colorCode);
    _frame_buffer.push_back(dmr_frame);

}

std::vector<DMRFrame> gr_dmr_dmo_sink::get_data()
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
