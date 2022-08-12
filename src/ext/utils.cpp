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

void unpackBytes(unsigned char *bitbuf, const unsigned char *bytebuf, int bytecount)
{
    for(int i=0; i<bytecount; i++)
    {
        for(int j=0; j<8; j++)
        {
            bitbuf[i*8+j] = (bytebuf[i] & (128 >> j)) != 0;
        }
    }
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

float boundedRand(int range)
{
    int x = rand();
    long m = long(x) * long(range);
    return 0.002 * float(m >> 32) / 32768.0f;
}

void buildModeList(QVector<QString> *operating_modes)
{
    operating_modes->push_back("FM");
    operating_modes->push_back("Narrow FM");
    operating_modes->push_back("Wide FM");
    operating_modes->push_back("USB");
    operating_modes->push_back("LSB");
    operating_modes->push_back("FreeDV1600 USB");
    operating_modes->push_back("FreeDV700C USB");
    operating_modes->push_back("FreeDV700D USB");
    operating_modes->push_back("FreeDV800XA USB");
    operating_modes->push_back("FreeDV1600 LSB");
    operating_modes->push_back("FreeDV700C LSB");
    operating_modes->push_back("FreeDV700D LSB");
    operating_modes->push_back("FreeDV800XA LSB");
    operating_modes->push_back("AM");
    operating_modes->push_back("BPSK 2K");
    operating_modes->push_back("BPSK 1K");
    operating_modes->push_back("QPSK 2K");
    operating_modes->push_back("QPSK 10K");
    operating_modes->push_back("2FSK RRC 2K");
    operating_modes->push_back("2FSK RRC 1K");
    operating_modes->push_back("2FSK 2K");
    operating_modes->push_back("2FSK 1K");
    operating_modes->push_back("2FSK RRC 10K");
    operating_modes->push_back("GMSK 2K");
    operating_modes->push_back("GMSK 1K");
    operating_modes->push_back("GMSK 10K");
    operating_modes->push_back("4FSK 2K");
    //operating_modes->push_back("4FSK 10K");
    operating_modes->push_back("4FSK RRC 2K");
    operating_modes->push_back("4FSK RRC 1K");
    operating_modes->push_back("4FSK RRC 10K");
    operating_modes->push_back("Video QPSK 250K");
    operating_modes->push_back("IP QPSK 250K");
    operating_modes->push_back("IP 4FSK 100K");
    operating_modes->push_back("Test tone");
    operating_modes->push_back("PSK8");
    operating_modes->push_back("MMDVM");
    operating_modes->push_back("MMDVM multi");
}

void buildFilterWidthList(std::vector<std::complex<int>>* filter_widths, std::vector<std::complex<int>>*ranges, std::vector<bool> *symmetric)
{
    filter_widths->push_back(std::complex<int>(-6500, 6500));  // FM
    filter_widths->push_back(std::complex<int>(-2500, 2500));  // NBFM
    filter_widths->push_back(std::complex<int>(-100000, 100000));  // WFM
    filter_widths->push_back(std::complex<int>(100, 2700)); // USB
    filter_widths->push_back(std::complex<int>(-2700, -100)); // LSB
    filter_widths->push_back(std::complex<int>(800, 2200)); // FreeDV1600 USB
    filter_widths->push_back(std::complex<int>(700, 2300)); // FreeDV700C USB
    filter_widths->push_back(std::complex<int>(900, 2100)); // FreeDV700D USB
    filter_widths->push_back(std::complex<int>(-1, 2500)); // FreeDV800XA USB
    filter_widths->push_back(std::complex<int>(-2200, -800)); // FreeDV1600 LSB
    filter_widths->push_back(std::complex<int>(-2300, -700)); // FreeDV700C LSB
    filter_widths->push_back(std::complex<int>(-2100, -900)); // FreeDV700D LSB
    filter_widths->push_back(std::complex<int>(-2500, 1)); // FreeDV800XA LSB
    filter_widths->push_back(std::complex<int>(-5000, 5000));  // AM
    filter_widths->push_back(std::complex<int>(-2800, 2800)); // BPSK 2K
    filter_widths->push_back(std::complex<int>(-1400, 1400)); // BPSK 700
    filter_widths->push_back(std::complex<int>(-1500, 1500));  // QPSK 2K
    filter_widths->push_back(std::complex<int>(-7000, 7000));    // QPSK 10K
    filter_widths->push_back(std::complex<int>(-2700, 2700));  // 2FSK 2K RRC
    filter_widths->push_back(std::complex<int>(-1350, 1350));  // 2FSK 1K RRC
    filter_widths->push_back(std::complex<int>(-4000, 4000));  // 2FSK 2K
    filter_widths->push_back(std::complex<int>(-2000, 2000));  // 2FSK 1K
    filter_widths->push_back(std::complex<int>(-15000, 15000));  // 2FSK 10K RRC
    filter_widths->push_back(std::complex<int>(-3000, 3000));  // GMSK 2K
    filter_widths->push_back(std::complex<int>(-1500, 1500));  // GMSK 1K
    filter_widths->push_back(std::complex<int>(-15000, 15000));  // GMSK 10K
    filter_widths->push_back(std::complex<int>(-4400, 4400));  // 4FSK 2K
    //filter_widths->push_back(std::complex<int>(-25000, 25000));    // 4FSK 10K
    filter_widths->push_back(std::complex<int>(-2500, 2500));  // 4FSK 2K RRC
    filter_widths->push_back(std::complex<int>(-1250, 1250));  // 4FSK 1K RRC
    filter_widths->push_back(std::complex<int>(-12500, 12500));    // 4FSK 10K RRC
    filter_widths->push_back(std::complex<int>(-150000, 150000)); // QPSK 250K VIDEO
    filter_widths->push_back(std::complex<int>(-150000, 150000)); // QPSK 250K DATA
    filter_widths->push_back(std::complex<int>(-125000, 125000)); // 4FSK 100K DATA
    filter_widths->push_back(std::complex<int>(100, 1000)); // CW K USB
    filter_widths->push_back(std::complex<int>(-150, 150));  // PSK8
    filter_widths->push_back(std::complex<int>(-6250, 6250));  // MMDVM
    filter_widths->push_back(std::complex<int>(-6250, 6250));  // MMDVM multi channel

    ranges->push_back(std::complex<int>(-10000, 10000));  // FM
    ranges->push_back(std::complex<int>(-9000, 9000));  // NBFM
    ranges->push_back(std::complex<int>(-100000, 100000));  // WFM
    ranges->push_back(std::complex<int>(200, 3800)); // USB
    ranges->push_back(std::complex<int>(-3800, -200)); // LSB
    ranges->push_back(std::complex<int>(800, 2200)); // FreeDV1600 USB
    ranges->push_back(std::complex<int>(700, 2300)); // FreeDV700C USB
    ranges->push_back(std::complex<int>(900, 2100)); // FreeDV700D USB
    ranges->push_back(std::complex<int>(200, 2500)); // FreeDV800XA USB
    ranges->push_back(std::complex<int>(-2200, -800)); // FreeDV1600 LSB
    ranges->push_back(std::complex<int>(-2300, -700)); // FreeDV700C LSB
    ranges->push_back(std::complex<int>(-2100, -900)); // FreeDV700D LSB
    ranges->push_back(std::complex<int>(-2500, -200)); // FreeDV800XA LSB
    ranges->push_back(std::complex<int>(-9000, 9000));  // AM
    ranges->push_back(std::complex<int>(-2800, 2800)); // BPSK 2K
    ranges->push_back(std::complex<int>(-1400, 1400)); // BPSK 700
    ranges->push_back(std::complex<int>(-1500, 1500));  // QPSK 2K
    ranges->push_back(std::complex<int>(-7000, 7000));    // QPSK 10K
    ranges->push_back(std::complex<int>(-2700, 2700));  // 2FSK 2K RRC
    ranges->push_back(std::complex<int>(-1350, 1350));  // 2FSK 1K RRC
    ranges->push_back(std::complex<int>(-4000, 4000));  // 2FSK 2K
    ranges->push_back(std::complex<int>(-2000, 2000));  // 2FSK 1K
    ranges->push_back(std::complex<int>(-15000, 15000));  // 2FSK 10K
    ranges->push_back(std::complex<int>(-3000, 3000));  // GMSK 2K
    ranges->push_back(std::complex<int>(-1500, 1500));  // GMSK 1K
    ranges->push_back(std::complex<int>(-15000, 15000));  // GMSK 10K
    ranges->push_back(std::complex<int>(-4400, 4400));  // 4FSK 2K
    //ranges->push_back(std::complex<int>(-25000, 25000));    // 4FSK 10K
    ranges->push_back(std::complex<int>(-2000, 2000));  // 4FSK 2K RRC
    ranges->push_back(std::complex<int>(-1000, 1000));  // 4FSK 1K RRC
    ranges->push_back(std::complex<int>(-7500, 7500));    // 4FSK 10K RRC
    ranges->push_back(std::complex<int>(-150000, 150000)); // QPSK 250K VIDEO
    ranges->push_back(std::complex<int>(-150000, 150000)); // QPSK 250K DATA
    ranges->push_back(std::complex<int>(-100000, 100000)); // 4FSK 100K DATA
    ranges->push_back(std::complex<int>(100, 1000)); // CW K USB
    ranges->push_back(std::complex<int>(-150, 150));  // PSK8
    ranges->push_back(std::complex<int>(-10000, 10000));  // MMDVM
    ranges->push_back(std::complex<int>(-10000, 10000));  // MMDVM multi channel

    symmetric->push_back(true);  // FM
    symmetric->push_back(true);  // NBFM
    symmetric->push_back(true);  // WFM
    symmetric->push_back(false); // USB
    symmetric->push_back(false); // LSB
    symmetric->push_back(false); // FreeDV1600 USB
    symmetric->push_back(false); // FreeDV700C USB
    symmetric->push_back(false); // FreeDV700D USB
    symmetric->push_back(false); // FreeDV800XA USB
    symmetric->push_back(false); // FreeDV1600 LSB
    symmetric->push_back(false); // FreeDV700C LSB
    symmetric->push_back(false); // FreeDV700D LSB
    symmetric->push_back(false); // FreeDV800XA LSB
    symmetric->push_back(true);  // AM
    symmetric->push_back(true); // BPSK 2K
    symmetric->push_back(true); // BPSK 700
    symmetric->push_back(true);  // QPSK 2K
    symmetric->push_back(true);    // QPSK 10K
    symmetric->push_back(true);  // 2FSK 2K
    symmetric->push_back(true);  // 2FSK 1K
    symmetric->push_back(true);  // 2FSK 2K FM
    symmetric->push_back(true);  // 2FSK 1K FM
    symmetric->push_back(true);  // 2FSK 10K
    symmetric->push_back(true);  // GMSK 2K
    symmetric->push_back(true);  // GMSK 1K
    symmetric->push_back(true);  // GMSK 10K
    symmetric->push_back(true);  // 4FSK 2K
    //symmetric->push_back(true);    // 4FSK 10K
    symmetric->push_back(true);  // 4FSK 2K RRC
    symmetric->push_back(true);  // 4FSK 1K RRC
    symmetric->push_back(true);    // 4FSK 10K RRC
    symmetric->push_back(true); // QPSK 250K VIDEO
    symmetric->push_back(true); // QPSK 250K DATA
    symmetric->push_back(true); // 4FSK 100K DATA
    symmetric->push_back(false); // CW K USB
    symmetric->push_back(true); // PSK8
    symmetric->push_back(true); // MMDVM
    symmetric->push_back(true); // MMDVM multi channel
}

