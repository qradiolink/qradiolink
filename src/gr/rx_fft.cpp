/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
 * Modified 2019 by Adrian Musceac YO8RZZ
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include <math.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/fft/fft.h>
#include "rx_fft.h"


rx_fft_c_sptr make_rx_fft_c (unsigned int fftsize, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft_c (fftsize, wintype));
}

/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr::filter::firdes::win_type).
 *
 */
rx_fft_c::rx_fft_c(unsigned int fftsize, int wintype)
    : gr::sync_block ("rx_fft_c",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(0, 0, 0)),
      d_fftsize(fftsize),
      d_wintype(-1)
{

    /* create FFT object */
    d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
    // must remember to check set_fft_size(), we malloc and free there as well
    d_fft_points = (float*)volk_malloc((size_t)d_fftsize * sizeof(float), volk_get_alignment());
    d_sample_buffer = d_fft->get_inbuf();
    d_counter = 0;
    d_data_ready = false;
    d_enabled = false;
    d_push = 0;

    /* create FFT window */
    set_window_type(wintype);
}

rx_fft_c::~rx_fft_c()
{
    delete d_fft;
    volk_free(d_fft_points);
}


int rx_fft_c::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    int i;
    const gr_complex *in = (const gr_complex*)input_items[0];
    (void) output_items;

    boost::mutex::scoped_lock lock(d_mutex);
    if(d_push > 0 || !d_enabled) // not sure if 0 or 1
    {
        // not consuming fast enough, no use filling the FFT buffer if no-one is reading it
        return noutput_items;
    }
    for (i = 0; i < noutput_items; i++)
    {
        if(d_counter >= d_fftsize)
        {
            d_counter = 0;
            d_fft->execute();
            volk_32fc_s32f_power_spectrum_32f_a(d_fft_points, d_fft->get_outbuf(), d_fftsize, d_fftsize);
            d_data_ready = true;
            d_push++;
        }
        d_sample_buffer[d_counter] = in[i] * d_window[d_counter]; // this is actually FFT's own buffer
        d_counter++;
    }

    return noutput_items;

}

void rx_fft_c::set_enabled(bool enabled)
{
    // no use having a copy block running at high sample rates
    boost::mutex::scoped_lock lock(d_mutex);
    d_enabled = enabled;
}

/*! \brief Get FFT data.
 *  \param fftPoints Buffer to copy FFT data
 *  \param fftSize Current FFT size (output).
 */
void rx_fft_c::get_fft_data(float* fftPoints, unsigned int &fftSize)
{
    boost::mutex::scoped_lock lock(d_mutex);
    d_push = 0; // want more samples in the FFT
    if (!d_data_ready)
    {
        // not enough samples in the buffer
        fftSize = 0;
        return;
    }
    // Shift FFT
    memcpy(fftPoints+(d_fftsize/2), d_fft_points, sizeof(float)*(d_fftsize/2));
    memcpy(fftPoints, d_fft_points+(d_fftsize/2), sizeof(float)*(d_fftsize/2));
    fftSize = d_fftsize;
    d_data_ready = false;
}

/*! \brief Set new FFT size. */
void rx_fft_c::set_fft_size(unsigned int fftsize)
{
    if (fftsize != d_fftsize)
    {
        boost::mutex::scoped_lock lock(d_mutex);

        d_fftsize = fftsize;

        /* clear and resize circular buffer */

        volk_free(d_fft_points);
        // eh, we should probably allocate the maximum possible size and
        // keep track instead of allocating each time
        d_fft_points = (float*)volk_malloc((size_t)d_fftsize * sizeof(float), volk_get_alignment());
        d_counter = 0;
        d_data_ready = false;
        /* reset window */
        int wintype = d_wintype; // FIXME: would be nicer with a window_reset()
        d_wintype = -1;
        set_window_type(wintype);

        /* reset FFT object (also reset FFTW plan) */
        delete d_fft;
        d_fft = new gr::fft::fft_complex_fwd (d_fftsize);
        d_sample_buffer = d_fft->get_inbuf();
    }

}

/*! \brief Get currently used FFT size. */
unsigned int rx_fft_c::get_fft_size() const
{
    return d_fftsize;
}

/*! \brief Set new window type. */
void rx_fft_c::set_window_type(int wintype)
{
    if (wintype == d_wintype)
    {
        /* nothing to do */
        return;
    }

    d_wintype = wintype;

    if ((d_wintype < gr::fft::window::WIN_HAMMING) || (d_wintype > gr::fft::window::WIN_FLATTOP))
    {
        d_wintype = gr::fft::window::WIN_HAMMING;
    }

    d_window.clear();
    d_window = gr::filter::firdes::window((gr::fft::window::win_type)d_wintype, d_fftsize, 6.76);
}

/*! \brief Get currently used window type. */
int rx_fft_c::get_window_type() const
{
    return d_wintype;
}


/**   rx_fft_f     **/

rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft_f (fftsize, wintype));
}

/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr::filter::firdes::win_type).
 *
 */
rx_fft_f::rx_fft_f(unsigned int fftsize, int wintype)
    : gr::sync_block ("rx_fft_f",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(0, 0, 0)),
      d_fftsize(fftsize),
      d_wintype(-1)
{

    /* create FFT object */
    d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
    d_cbuf = new boost::circular_buffer<float>;

    /* allocate circular buffer */
    d_cbuf->set_capacity(d_fftsize);

    /* create FFT window */
    set_window_type(wintype);
}

rx_fft_f::~rx_fft_f()
{
    delete d_fft;
    delete d_cbuf;
}

/*! \brief Audio FFT work method.
 *  \param noutput_items
 *  \param input_items
 *  \param output_items
 *
 * This method does nothing except throwing the incoming samples into the
 * circular buffer.
 * FFT is only executed when the GUI asks for new FFT data via get_fft_data().
 */
int rx_fft_f::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    int i;
    const float *in = (const float*)input_items[0];
    (void) output_items;

    /* just throw new samples into the buffer */
    boost::mutex::scoped_lock lock(d_mutex);
    for (i = 0; i < noutput_items; i++)
    {
        d_cbuf->push_back(in[i]);
    }

    return noutput_items;
}

/*! \brief Get FFT data.
 *  \param fftPoints Buffer to copy FFT data
 *  \param fftSize Current FFT size (output).
 */
void rx_fft_f::get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize)
{
    boost::mutex::scoped_lock lock(d_mutex);

    if (d_cbuf->size() < d_fftsize)
    {
        // not enough samples in the buffer
        fftSize = 0;

        return;
    }

    /* perform FFT */
    do_fft(d_cbuf->linearize(), d_cbuf->size());  // FIXME: array_one() and two() may be faster
    //d_cbuf->clear();

    /* get FFT data */
    memcpy(fftPoints, d_fft->get_outbuf(), sizeof(gr_complex)*d_fftsize);
    fftSize = d_fftsize;
}

/*! \brief Compute FFT on the available input data.
 *  \param data_in The data to compute FFT on.
 *  \param size The size of data_in.
 *
 * Note that this function does not lock the mutex since the caller, get_fft_data()
 * has alrady locked it.
 */
void rx_fft_f::do_fft(const float *data_in, unsigned int size)
{
    gr_complex *dst = d_fft->get_inbuf();
    unsigned int i;

    /* apply window, and convert to complex */
    if (d_window.size())
    {
        for (i = 0; i < size; i++)
            dst[i] = data_in[i] * d_window[i];
    }
    else
    {
        for (i = 0; i < size; i++)
            dst[i] = data_in[i];
    }

    /* compute FFT */
    d_fft->execute();
}


/*! \brief Set new FFT size. */
void rx_fft_f::set_fft_size(unsigned int fftsize)
{
    if (fftsize != d_fftsize)
    {
        boost::mutex::scoped_lock lock(d_mutex);

        d_fftsize = fftsize;

        /* clear and resize circular buffer */
        d_cbuf->clear();
        d_cbuf->set_capacity(d_fftsize);

        /* reset window */
        int wintype = d_wintype; // FIXME: would be nicer with a window_reset()
        d_wintype = -1;
        set_window_type(wintype);

        /* reset FFT object (also reset FFTW plan) */
        delete d_fft;
        d_fft = new gr::fft::fft_complex_fwd(d_fftsize, true);
    }
}

/*! \brief Get currently used FFT size. */
unsigned int rx_fft_f::get_fft_size() const
{
    return d_fftsize;
}

/*! \brief Set new window type. */
void rx_fft_f::set_window_type(int wintype)
{
    if (wintype == d_wintype)
    {
        /* nothing to do */
        return;
    }

    d_wintype = wintype;

    if ((d_wintype < gr::fft::window::WIN_HAMMING) || (d_wintype > gr::fft::window::WIN_FLATTOP))
    {
        d_wintype = gr::fft::window::WIN_HAMMING;
    }

    d_window.clear();
    d_window = gr::filter::firdes::window((gr::fft::window::win_type)d_wintype, d_fftsize, 6.76);
}

/*! \brief Get currently used window type. */
int rx_fft_f::get_window_type() const
{
    return d_wintype;
}
