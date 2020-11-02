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


#ifndef INCLUDED_CESSB_STRETCHER_CC_H
#define INCLUDED_CESSB_STRETCHER_CC_H

#include "api.h"
#include <gnuradio/block.h>

namespace gr {
  namespace cessb {

    /*!
     * \brief <+description of block+>
     * \ingroup cessb
     *
     */
    class CESSB_API stretcher_cc : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<stretcher_cc> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of cessb::stretcher_cc.
       *
       * To avoid accidental use of raw pointers, cessb::stretcher_cc's
       * constructor is in a private implementation
       * class. cessb::stretcher_cc::make is the public interface for
       * creating new instances.
       */
      static sptr make();
    };

  } // namespace cessb
} // namespace gr

#endif /* INCLUDED_CESSB_STRETCHER_CC_H */

