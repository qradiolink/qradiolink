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
#ifndef RX_FFT_H
#define RX_FFT_H

#include <gnuradio/sync_block.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/filter/firdes.h>       /* contains enum win_type */
#include <gnuradio/gr_complex.h>
#include <boost/thread/mutex.hpp>
#include <boost/circular_buffer.hpp>
#include <volk/volk.h>


#define MAX_FFT_SIZE 1048576

class rx_fft_c;
class rx_fft_f;

typedef std::shared_ptr<rx_fft_c> rx_fft_c_sptr;
typedef std::shared_ptr<rx_fft_f> rx_fft_f_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_fft_c.
 *  \param fftsize The FFT size
 *  \param winttype The window type (see gnuradio/filter/firdes.h)
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_fft_c constructor is private.
 * make_rx_fft_c is the public interface for creating new instances.
 */
rx_fft_c_sptr make_rx_fft_c(unsigned int fftsize=4096, int wintype=gr::fft::window::WIN_HAMMING);


/*! \brief Block for computing complex FFT.
 *  \ingroup DSP
 *
 * This block is used to compute the FFT of the received spectrum.
 *
 * The samples are collected in a cicular buffer with size FFT_SIZE.
 * When the GUI asks for a new set of FFT data via get_fft_data() an FFT
 * will be performed on the data stored in the circular buffer - assuming
 * of course that the buffer contains at least fftsize samples.
 *
 * \note Uses code from qtgui_sink_c
 */
class rx_fft_c : public gr::sync_block
{
    friend rx_fft_c_sptr make_rx_fft_c(unsigned int fftsize, int wintype);

protected:
    rx_fft_c(unsigned int fftsize=4096, int wintype=gr::fft::window::WIN_HAMMING);

public:
    ~rx_fft_c();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    void get_fft_data(float *fftPoints, unsigned int &fftSize);

    void set_window_type(int wintype);
    int  get_window_type() const;

    void set_fft_size(unsigned int fftsize);
    unsigned int get_fft_size() const;
    void set_enabled(bool enabled);

private:
    unsigned int d_fftsize;   /*! Current FFT size. */
    int          d_wintype;   /*! Current window type. */

    boost::mutex d_mutex;  /*! Used to lock FFT output buffer. */

    gr::fft::fft_complex_fwd    *d_fft;    /*! FFT object. */
    std::vector<float>  d_window; /*! FFT window taps. */

    float *d_fft_points;

    gr_complex *d_sample_buffer;

    unsigned int d_counter;
    bool d_data_ready;
    int d_push;
    bool d_enabled;

    void do_fft(const gr_complex *data_in, unsigned int size);

};


/*! \brief Return a shared_ptr to a new instance of rx_fft_f.
 *  \param fftsize The FFT size
 *  \param winttype The window type (see gnuradio/filter/firdes.h)
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_fft_f constructor is private.
 * make_rx_fft_f is the public interface for creating new instances.
 */
rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize=1024, int wintype=gr::fft::window::WIN_HAMMING);


/*! \brief Block for computing real FFT.
 *  \ingroup DSP
 *
 * This block is used to compute the FFT of the audio spectrum or anything
 * else where real FFT is useful.
 *
 * The samples are collected in a cicular buffer with size FFT_SIZE.
 * When the GUI asks for a new set of FFT data using get_fft_data() an FFT
 * will be performed on the data stored in the circular buffer - assuming
 * that the buffer contains at least fftsize samples.
 *
 * \note Uses code from qtgui_sink_f
 */
class rx_fft_f : public gr::sync_block
{
    friend rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize, int wintype);

protected:
    rx_fft_f(unsigned int fftsize=1024, int wintype=gr::fft::window::WIN_HAMMING);

public:
    ~rx_fft_f();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    void get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize);

    void set_window_type(int wintype);
    int  get_window_type() const;

    void set_fft_size(unsigned int fftsize);
    unsigned int get_fft_size() const;

private:
    unsigned int d_fftsize;   /*! Current FFT size. */
    int          d_wintype;   /*! Current window type. */

    boost::mutex d_mutex;  /*! Used to lock FFT output buffer. */

    gr::fft::fft_complex_fwd    *d_fft;    /*! FFT object. */
    std::vector<float>  d_window; /*! FFT window taps. */

    boost::circular_buffer<float> *d_cbuf; /*! buffer to accumulate samples. */

    void do_fft(const float *data_in, unsigned int size);

};


#endif /* RX_FFT_H */
