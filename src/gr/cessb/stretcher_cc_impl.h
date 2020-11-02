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

#ifndef INCLUDED_CESSB_STRETCHER_CC_IMPL_H
#define INCLUDED_CESSB_STRETCHER_CC_IMPL_H

#include "stretcher_cc.h"

#define CHUNK_SIZE 1024

namespace gr {
  namespace cessb {

    class stretcher_cc_impl : public stretcher_cc
    {
     private:
      float d_env[CHUNK_SIZE + 4];
      float d_envhold[CHUNK_SIZE];
      float d_ones[CHUNK_SIZE];
      float d_real[CHUNK_SIZE];
      float d_imag[CHUNK_SIZE];

     public:
      stretcher_cc_impl();
      ~stretcher_cc_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace cessb
} // namespace gr

#endif /* INCLUDED_CESSB_STRETCHER_CC_IMPL_H */

