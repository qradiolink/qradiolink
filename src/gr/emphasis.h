/**
    #
    # Copyright 2005,2007,2012 Free Software Foundation, Inc.
    #
    # This file is part of GNU Radio
    #
    # SPDX-License-Identifier: GPL-3.0-or-later
    #
    #
*/

#ifndef EMPHASIS_H
#define EMPHASIS_H
#include <vector>

namespace gr {


void calculate_deemph_taps(int sample_rate, double tau, std::vector<double>& ataps,
                           std::vector<double>& btaps);
void calculate_preemph_taps(int sample_rate, double tau, std::vector<double>& ataps,
                            std::vector<double>& btaps,
                            double fh=-1.0);
}
#endif // EMPHASIS_H
