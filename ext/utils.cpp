/* Copyright (C) 2009-2013, Martin Johansson <martin@fatbob.nu>
   Copyright (C) 2005-2013, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Developers nor the names of its contributors may
     be used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "utils.h"


void addPreamble(quint8 *buffer, quint16 type, quint32 len)
{
    buffer[1] = (type) & 0xff;
    buffer[0] = (type >> 8) & 0xff;

    buffer[5] = (len) & 0xff;
    buffer[4] = (len >> 8) & 0xff;
    buffer[3] = (len >> 16) & 0xff;
    buffer[2] = (len >> 24) & 0xff;
}

void getPreamble(quint8 *buffer, int *type, int *len)
{
    quint16 msgType;
    quint32 msgLen;

    msgType = buffer[1] | (buffer[0] << 8);
    msgLen = buffer[5] | (buffer[4] << 8) | (buffer[3] << 16) | (buffer[2] << 24);
    *type = (int)msgType;
    *len = (int)msgLen;
}

void genRandomStr(char *str, const int len)
{
    static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand(time(0));
    for (int i = 0; i < len; ++i)
    {
        str[i] = letters[rand() % (sizeof(letters) - 1)];
    }

    str[len] = 0;
}

std::vector<std::complex<int>>* buildFilterWidthList()
{
    std::vector<std::complex<int>> *filter_widths = new std::vector<std::complex<int>>;
    filter_widths->push_back(std::complex<int>(-5000, 5000));  // FM
    filter_widths->push_back(std::complex<int>(-2500, 2500));  // NBFM
    filter_widths->push_back(std::complex<int>(-100000, 100000));  // WFM
    filter_widths->push_back(std::complex<int>(-1, 2500)); // USB
    filter_widths->push_back(std::complex<int>(-2500, 1)); // LSB
    filter_widths->push_back(std::complex<int>(-1, 2500)); // FreeDV1600 USB
    filter_widths->push_back(std::complex<int>(-1, 2500)); // FreeDV700C USB
    filter_widths->push_back(std::complex<int>(-1, 2500)); // FreeDV800XA USB
    filter_widths->push_back(std::complex<int>(-2500, 1)); // FreeDV1600 LSB
    filter_widths->push_back(std::complex<int>(-2500, 1)); // FreeDV700C LSB
    filter_widths->push_back(std::complex<int>(-2500, 1)); // FreeDV800XA LSB
    filter_widths->push_back(std::complex<int>(-5000, 5000));  // AM
    filter_widths->push_back(std::complex<int>(-2800, 2800)); // BPSK 2K
    filter_widths->push_back(std::complex<int>(-1400, 1400)); // BPSK 700
    filter_widths->push_back(std::complex<int>(-1500, 1500));  // QPSK 2K
    filter_widths->push_back(std::complex<int>(-7000, 7000));    // QPSK 10K
    filter_widths->push_back(std::complex<int>(-4600, 4600));  // 2FSK 2K
    filter_widths->push_back(std::complex<int>(-15000, 15000));  // 2FSK 10K
    filter_widths->push_back(std::complex<int>(-4600, 4600));  // 4FSK 2K
    filter_widths->push_back(std::complex<int>(-25000, 25000));    // 4FSK 10K
    filter_widths->push_back(std::complex<int>(-150000, 150000)); // QPSK250000 VIDEO
    filter_widths->push_back(std::complex<int>(-150000, 150000)); // QPSK250000 DATA
    return filter_widths;
}



