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
#include "stretcher_cc_impl.h"
#include <volk/volk.h>
#include <gnuradio/math.h>

namespace gr {
  namespace cessb {

    stretcher_cc::sptr
    stretcher_cc::make()
    {
      return gnuradio::get_initial_sptr
        (new stretcher_cc_impl());
    }

    /*
     * The private constructor
     */
    stretcher_cc_impl::stretcher_cc_impl()
      : gr::block("stretcher_cc",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {
      d_env[0] = 0.0;
      d_env[1] = 0.0;
      for (int i = 0; i < CHUNK_SIZE; i++)
      {
          d_ones[i] = 1.0;
      }
      set_output_multiple(CHUNK_SIZE);
    }

    /*
     * Our virtual destructor.
     */
    stretcher_cc_impl::~stretcher_cc_impl()
    {
    }

    void
    stretcher_cc_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items + ((noutput_items / CHUNK_SIZE) * 2);
    }

    int
    stretcher_cc_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];
      const float emax = 1 / (sqrt(0.5) / 2);

      for (int i = 0; i < noutput_items; i += CHUNK_SIZE)
      {
          volk_32fc_magnitude_32f(d_env + 2, in, CHUNK_SIZE + 2);
          memcpy(d_envhold, d_env + 2, (CHUNK_SIZE) * sizeof(float));
          volk_32f_x2_max_32f(d_envhold, d_envhold, d_env, CHUNK_SIZE);
          volk_32f_x2_max_32f(d_envhold, d_envhold, d_env + 1, CHUNK_SIZE);
          volk_32f_x2_max_32f(d_envhold, d_envhold, d_env + 3, CHUNK_SIZE);
          volk_32f_x2_max_32f(d_envhold, d_envhold, d_env + 4, CHUNK_SIZE);
          volk_32f_s32f_multiply_32f(d_envhold, d_envhold, emax, CHUNK_SIZE);
          volk_32f_x2_max_32f(d_envhold, d_envhold, d_ones, CHUNK_SIZE);
          volk_32f_x2_subtract_32f(d_envhold, d_envhold, d_ones, CHUNK_SIZE);
          volk_32f_s32f_multiply_32f(d_envhold, d_envhold, 2.0, CHUNK_SIZE);
          volk_32f_x2_add_32f(d_envhold, d_envhold, d_ones, CHUNK_SIZE);
          volk_32fc_deinterleave_real_32f(d_real, in, CHUNK_SIZE);
          volk_32fc_deinterleave_imag_32f(d_imag, in, CHUNK_SIZE);
          volk_32f_x2_divide_32f(d_real, d_real, d_envhold, CHUNK_SIZE);
          volk_32f_x2_divide_32f(d_imag, d_imag, d_envhold, CHUNK_SIZE);
          volk_32f_x2_interleave_32fc(out, d_real, d_imag, CHUNK_SIZE);
          d_env[0] = d_env[CHUNK_SIZE];
          d_env[1] = d_env[CHUNK_SIZE + 1];
          in += CHUNK_SIZE;
          out += CHUNK_SIZE;
      }

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace cessb */
} /* namespace gr */

