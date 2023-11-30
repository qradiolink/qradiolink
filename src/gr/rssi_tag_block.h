// Written by Adrian Musceac YO8RZZ , started Nov 2023.
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

#ifndef RSSI_TAG_BLOCK_H
#define RSSI_TAG_BLOCK_H


#include <gnuradio/sync_block.h>

class rssi_tag_block;

typedef boost::shared_ptr<rssi_tag_block> rssi_tag_block_sptr;

rssi_tag_block_sptr make_rssi_tag_block();

class rssi_tag_block : public gr::sync_block
{

public:
    rssi_tag_block();
    ~rssi_tag_block();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);
    void calibrate_rssi(float level);

private:
    void add_rssi_tag(float db);
    float _calibration_level;

};


#endif // RSSI_TAG_BLOCK_H
