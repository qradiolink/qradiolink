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

#include <cstdio>

#include <gnuradio/io_signature.h>
#include "dsss_decoder_cc_impl.h"

#include <gnuradio/filter/fir_filter.h>
#include <gnuradio/filter/firdes.h>

namespace gr {
namespace dsss {

dsss_decoder_cc::sptr dsss_decoder_cc::make(const std::vector<int> &code, float samples_per_symbol)
{
    return gnuradio::get_initial_sptr
            (new dsss_decoder_cc_impl(code, samples_per_symbol));
}

/*
     * The private constructor
     */
dsss_decoder_cc_impl::dsss_decoder_cc_impl(const std::vector<int> &code, float samples_per_symbol)
    : gr::block("dsss_decoder_cc",
                gr::io_signature::make(1, 1, sizeof(gr_complex)),
                gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
    set_code(code);
    d_samples_per_symbol = samples_per_symbol;
    /// TODO: QPSK
    /// int bits_per_symbol = 1;
    /// int arity = pow(2, bits_per_symbol);

    int samples = (int) d_samples_per_symbol;

    int rrc_ntaps = samples * 11;


    int code_symbols_size = d_code.size() * samples;
    int extra_symbols = rrc_ntaps;
    gr_complex* code_symbols = new gr_complex[code_symbols_size + 2*extra_symbols];

    for (int i = 0; i < extra_symbols; i++) {
        code_symbols[i] = 0;
    }
    for (int i = extra_symbols + d_code.size() * samples; i < code_symbols_size + 2*extra_symbols; i++) {
        code_symbols[i] = 0;
    }

    for (unsigned int i = 0; i < d_code.size(); i++) {
        gr_complex c = d_code[d_code.size() - (i + 1)] == 0 ? -1.0 : 1.0;

        code_symbols[extra_symbols + i * samples] = c;

        for (int k = 1; k < samples; k++) {
            code_symbols[extra_symbols + i * samples + k] = c;
        }
    }



    float excess_bw = 0.350f;
    static std::vector<float> rrc_taps =
            filter::firdes::root_raised_cosine(1, // gain
                                               d_samples_per_symbol, // sampling rate
                                               1.0, // symbol rate
                                               excess_bw, // roll-off factor
                                               rrc_ntaps);

    filter::kernel::fir_filter_ccf *fir_filter = new filter::kernel::fir_filter_ccf(rrc_taps);

    for (int i = 0; i < code_symbols_size + extra_symbols; i++) {
        d_taps.push_back(fir_filter->filter(&code_symbols[i]));
    }

    d_match_filter = new filter::kernel::fir_filter_ccc(d_taps);
    // we can use this to tag back in time
    set_history(d_samples_per_symbol * d_code.size());

    set_relative_rate(1/(d_code.size()*d_samples_per_symbol));

    // clean-up
    delete fir_filter;
    delete[] code_symbols;
}

/*
     * Our virtual destructor.
     */
dsss_decoder_cc_impl::~dsss_decoder_cc_impl()
{
    delete d_match_filter;
}

void
dsss_decoder_cc_impl::set_code(const std::vector<int> &code) {
    d_code = code;
}

void dsss_decoder_cc_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
{
    int code_sample_size = d_samples_per_symbol * d_code.size();
    ninput_items_required[0] = code_sample_size * noutput_items;
}

int dsss_decoder_cc_impl::general_work (int noutput_items,
                                    gr_vector_int &ninput_items,
                                    gr_vector_const_void_star &input_items,
                                    gr_vector_void_star &output_items)
{
    (void)ninput_items;
    const gr_complex *in = (const gr_complex *) input_items[0];
    gr_complex *out = (gr_complex *) output_items[0];

    int code_sample_size = d_samples_per_symbol * d_code.size();
    float max_abs_val, cur_abs_val;
    gr_complex max_val, cur_val;
    int consumed = 0;

    for (int i = 0; i < noutput_items; i++) {
        max_abs_val = 0;
        max_val = 0;

        // FIXME: Will not work for non-integer samples_per_symbols
        // FIXME: Will not work for large code sizes
        for (int j = 0; j < code_sample_size; j++) {
            cur_val = d_match_filter->filter(in + (i-1) * code_sample_size + j);
            cur_abs_val = abs(cur_val);
            if (cur_abs_val > max_abs_val) {
                max_abs_val = cur_abs_val;
                max_val = cur_val;
            }

            consumed++;
        }
        out[i] = max_val * (2.0f / code_sample_size );
    }

    // Tell runtime system how many input items we consumed on
    // each input stream.
    consume_each(consumed);

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

} /* namespace dsss */
} /* namespace gr */
