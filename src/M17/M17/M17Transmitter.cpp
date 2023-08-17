/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <M17/M17CodePuncturing.hpp>
#include <M17/M17Decorrelator.hpp>
#include <M17/M17Interleaver.hpp>
#include <M17/M17Transmitter.hpp>

using namespace M17;

M17Transmitter::M17Transmitter() : currentLich(0), frameNumber(0)
{

}

M17Transmitter::~M17Transmitter()
{

}

void M17Transmitter::start(const std::string& src, std::vector<unsigned char> *bytes)
{
    // Just call start() with an empty string for destination callsign.
    std::string empty;
    uint16_t CAN = 0;
    start(src, empty, CAN, 0, bytes);
}

void M17Transmitter::start(const std::string& src, const std::string& dst, uint16_t CAN, int destination_type,
                           std::vector<unsigned char> *bytes)
{
    // Reset LICH and frame counters
    currentLich = 0;
    frameNumber = 0;

    // Fill the Link Setup Frame
    lsf.clear();
    lsf.setSource(src);
    if(destination_type == 0)
    {
        if(!dst.empty()) lsf.setDestination(dst);
    }
    else
    {
        lsf.setDestination(destination_type);
    }

    streamType_t type;
    type.fields.stream   = 1;    // Stream
    type.fields.dataType = 2;    // Voice data
    type.fields.CAN      = CAN;  // Channel access number
    type.fields.encType  = 0;

    lsf.setType(type);
    lsf.updateCrc();

    // Generate the Golay(24,12) LICH segments
    for(size_t i = 0; i < lichSegments.size(); i++)
    {
        lichSegments[i] = lsf.generateLichSegment(i);
    }

    // Encode the LSF, then puncture and decorrelate its data
    std::array<uint8_t, 61> encoded;
    encoder.reset();
    encoder.encode(lsf.getData(), encoded.data(), sizeof(M17LinkSetupFrame));
    encoded[60] = encoder.flush();

    std::array<uint8_t, 46> punctured;
    puncture(encoded, punctured, LSF_PUNCTURE);
    interleave(punctured);
    decorrelate(punctured);

    // Send preamble
    std::array<uint8_t, 2>  preamble_sync;
    std::array<uint8_t, 46> preamble_bytes;
    preamble_sync.fill(0x77);
    preamble_bytes.fill(0x77);
    for(int i=0;i<preamble_sync.size();i++)
        bytes->push_back(preamble_sync.at(i));
    for(int i=0;i<preamble_bytes.size();i++)
        bytes->push_back(preamble_bytes.at(i));

    // Send LSF
    bytes->push_back(LSF_SYNC_WORD[0]);
    bytes->push_back(LSF_SYNC_WORD[1]);
    for(int i=0;i<punctured.size();i++)
        bytes->push_back(punctured.at(i));
}

void M17Transmitter::send(const payload_t& payload,
                          std::vector<unsigned char> *bytes,
                          const bool isLast)
{
    dataFrame.clear();
    dataFrame.setFrameNumber(frameNumber);
    frameNumber = (frameNumber + 1) & 0x07FF;
    if(isLast) dataFrame.lastFrame();
    std::copy(payload.begin(), payload.end(), dataFrame.payload().begin());

    // Encode frame
    std::array<uint8_t, 37> encoded;
    encoder.reset();
    encoder.encode(dataFrame.getData(), encoded.data(), sizeof(M17StreamFrame));
    encoded[36] = encoder.flush();

    std::array<uint8_t, 34> punctured;
    puncture(encoded, punctured, DATA_PUNCTURE);

    // Add LICH segment to coded data and send
    std::array<uint8_t, 46> frame;
    auto it = std::copy(lichSegments[currentLich].begin(),
                        lichSegments[currentLich].end(),
                        frame.begin());
    std::copy(punctured.begin(), punctured.end(), it);

    // Increment LICH counter after copy
    currentLich = (currentLich + 1) % lichSegments.size();

    interleave(frame);
    decorrelate(frame);
    bytes->push_back(STREAM_SYNC_WORD[0]);
    bytes->push_back(STREAM_SYNC_WORD[1]);
    for(int i=0;i<frame.size();i++)
        bytes->push_back(frame.at(i));
}
