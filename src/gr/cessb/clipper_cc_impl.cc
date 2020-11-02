/* -*- c++ -*- */
/* 
 * Copyright 2016 Ron Economos.
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
#include "clipper_cc_impl.h"
#include <volk/volk.h>
#include <gnuradio/math.h>

namespace gr {
  namespace cessb {

    clipper_cc::sptr
    clipper_cc::make(float clip)
    {
      return gnuradio::get_initial_sptr
        (new clipper_cc_impl(clip));
    }

    /*
     * The private constructor
     */
    clipper_cc_impl::clipper_cc_impl(float clip)
      : gr::sync_block("clipper_cc",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {
      const int alignment_multiple = volk_get_alignment() / sizeof(float);
      set_alignment(std::max(1, alignment_multiple));
      d_clip = clip;
      for (int i = 0; i < CHUNK_SIZE; i++)
      {
          d_cliplevel[i] = clip;
      }
      set_output_multiple(CHUNK_SIZE);
    }

    /*
     * Our virtual destructor.
     */
    clipper_cc_impl::~clipper_cc_impl()
    {
    }

    int
    clipper_cc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];

      for (int i = 0; i < noutput_items; i += CHUNK_SIZE)
      {
          volk_32fc_magnitude_32f(d_magnitude, in, CHUNK_SIZE);

          for (int j = 0; j < CHUNK_SIZE; j++)
          {
	      d_phase[j] = gr::fast_atan2f(in[j]);
          }

          volk_32f_x2_min_32f(d_clipped, d_magnitude, d_cliplevel, CHUNK_SIZE);

          volk_32f_cos_32f(d_phase_cos, d_phase, CHUNK_SIZE);
          volk_32f_sin_32f(d_phase_sin, d_phase, CHUNK_SIZE);
          volk_32f_x2_multiply_32f(d_phase_cos, d_phase_cos, d_clipped, CHUNK_SIZE);
          volk_32f_x2_multiply_32f(d_phase_sin, d_phase_sin, d_clipped, CHUNK_SIZE);
          volk_32f_x2_interleave_32fc(out, d_phase_cos, d_phase_sin, CHUNK_SIZE);
          in += CHUNK_SIZE;
          out += CHUNK_SIZE;
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace cessb */
} /* namespace gr */

