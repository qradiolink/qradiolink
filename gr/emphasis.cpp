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
#include <math.h>
#include "emphasis.h"

namespace gr {

void calculate_deemph_taps(int sample_rate, double tau, std::vector<double>& ataps,
                           std::vector<double>& btaps)
{
    // code from GNUradio gr-analog/python/analog/fm_emph.py

    double fs = (double) sample_rate;
    // Digital corner frequency

    double w_c = 1.0 / tau;

    // Prewarped analog corner frequency
    double w_ca = 2.0 * fs * tanf(w_c / (2.0 * fs));

    // Resulting digital pole, zero, and gain term from the bilinear
    // transformation of H(s) = w_ca / (s + w_ca) to
    // H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
    double k = -w_ca / (2.0 * fs);
    double z1 = -1.0;
    double p1 = (1.0 + k) / (1.0 - k);
    double b0 = -k / (1.0 - k);

    btaps = { b0 * 1.0, b0 * -z1 };
    ataps = {      1.0,      -p1 };

    // Since H(s = 0) = 1.0, then H(z = 1) = 1.0 and has 0 dB gain at DC

}

void calculate_preemph_taps(int sample_rate, double tau, std::vector<double>& ataps,
                            std::vector<double>& btaps, double fh)
{
    double fs = (double) sample_rate;
    // code from GNUradio gr-analog/python/analog/fm_emph.py

    // Set fh to something sensible, if needed.
    // N.B. fh == fs/2.0 or fh == 0.0 results in a pole on the unit circle
    // at z = -1.0 or z = 1.0 respectively.  That makes the filter unstable
    // and useless.
    if (fh <= 0.0 || fh >= fs / 2.0)
    {
        fh = 0.925 * fs/2.0;
    }

    // Digital corner frequencies
    double w_cl = 1.0 / tau;
    double w_ch = 2.0 * M_PI * fh;

    // Prewarped analog corner frequencies
    double w_cla = 2.0 * fs * tanf(w_cl / (2.0 * fs));
    double w_cha = 2.0 * fs * tanf(w_ch / (2.0 * fs));

    // Resulting digital pole, zero, and gain term from the bilinear
    // transformation of H(s) = (s + w_cla) / (s + w_cha) to
    // H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
    double kl = -w_cla / (2.0 * fs);
    double kh = -w_cha / (2.0 * fs);
    double z1 = (1.0 + kl) / (1.0 - kl);
    double p1 = (1.0 + kh) / (1.0 - kh);
    double b0 = (1.0 - kl) / (1.0 - kh);

    // Since H(s = infinity) = 1.0, then H(z = -1) = 1.0 and
    // this filter  has 0 dB gain at fs/2.0.
    // That isn't what users are going to expect, so adjust with a
    // gain, g, so that H(z = 1) = 1.0 for 0 dB gain at DC.
    double w_0dB = 2.0 * M_PI * 0.0;
    double g = fabs(1.0 - p1 * 1.0 * (cos(-w_0dB) + sin(-w_0dB)))
        / (b0 * fabs(1.0 - z1 * 1.0 * (cos(-w_0dB) + sin(-w_0dB))));

    btaps = { g * b0 * 1.0, g * b0 * -z1 };
    ataps = { 1.0, -p1 };


}

}
