/* -*- c++ -*- */
/* 
 * Copyright 2014 Eric de Groot (edegroot@email.arizona.edu).
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "dsss_encoder_bb_impl.h"

namespace gr {
namespace dsss {

dsss_encoder_bb::sptr dsss_encoder_bb::make(const std::vector<int> &code)
{
    return gnuradio::get_initial_sptr
            (new dsss_encoder_bb_impl(code));
}

/*
 * The private constructor
 */
dsss_encoder_bb_impl::dsss_encoder_bb_impl(const std::vector<int> &code)
    : gr::block("dsss_encoder_bb",
                gr::io_signature::make(1, 1, sizeof(unsigned char)),
                gr::io_signature::make(1, 1, sizeof(unsigned char)))
{
    set_code(code);
    set_relative_rate(code.size()*8);
    set_output_multiple(code.size()*8);
}

/*
 * Our virtual destructor.
 */
dsss_encoder_bb_impl::~dsss_encoder_bb_impl()
{
    /* NOP */
}

void dsss_encoder_bb_impl::set_code(const std::vector<int> &code) {
    d_code = code;
}

void dsss_encoder_bb_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
{
    ninput_items_required[0] = noutput_items / (8.0 * d_code.size());
}

int dsss_encoder_bb_impl::general_work (int noutput_items,
                                    gr_vector_int &ninput_items,
                                    gr_vector_const_void_star &input_items,
                                    gr_vector_void_star &output_items)
{
    (void)ninput_items;
    const unsigned char *in = (const unsigned char *) input_items[0];
    unsigned char *out = (unsigned char *) output_items[0];

    int input_required = noutput_items / (8 * d_code.size());


    for (int i = 0; i < input_required; i++) {
        for (int j = 0; j < 8; j++) {
            int b = *in & (1 << (7-j));

            for (std::vector<int>::iterator it = d_code.begin(); it != d_code.end(); ++it) {
                if (b == 0)
                    *out = 1 & (*it);
                else
                    *out = 1 & (~(*it));
                out++;
            }
        }

        in++;
    }

    // Tell runtime system how many input items we consumed on
    // each input stream.
    consume_each(input_required);

    // Tell runtime system how many output items we produced.
    return input_required*8*d_code.size();
}

} /* namespace dsss */
} /* namespace gr */
