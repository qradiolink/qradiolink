// Written by Adrian Musceac YO8RZZ , started March 2019.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "limits.h"

Limits::Limits()
{
    _allocation_name = "IARU region 1 / CEPT allocation";
    // TODO: configurable band allocation
    _tx_limits.push_back(std::complex<long long>(1810000, 2000000));
    _tx_limits.push_back(std::complex<long long>(3500000, 3800000));
    _tx_limits.push_back(std::complex<long long>(7000000, 7200000));
    _tx_limits.push_back(std::complex<long long>(10100000, 10150000));
    _tx_limits.push_back(std::complex<long long>(10100000, 10150000));
    _tx_limits.push_back(std::complex<long long>(14000000, 14350000));
    _tx_limits.push_back(std::complex<long long>(18068000, 18168000));
    _tx_limits.push_back(std::complex<long long>(21000000, 21450000));
    _tx_limits.push_back(std::complex<long long>(24890000, 24990000));
    _tx_limits.push_back(std::complex<long long>(28000000, 29700000));
    _tx_limits.push_back(std::complex<long long>(50000000, 52000000));
    _tx_limits.push_back(std::complex<long long>(70000000, 70300000));
    _tx_limits.push_back(std::complex<long long>(144000000, 146000000));
    _tx_limits.push_back(std::complex<long long>(430000000, 440000000));
    _tx_limits.push_back(std::complex<long long>(1240000000, 1300000000));
    _tx_limits.push_back(std::complex<long long>(2300000000, 2450000000));
    _tx_limits.push_back(std::complex<long long>(2300000000, 2450000000));
    _tx_limits.push_back(std::complex<long long>(3400000000, 3410000000));
    _tx_limits.push_back(std::complex<long long>(5660000000, 5670000000));
    _tx_limits.push_back(std::complex<long long>(5725000000, 5850000000));
    _tx_limits.push_back(std::complex<long long>(10000000000, 10300000000));
}

bool Limits::checkLimit(long long tx_freq)
{
    /// this doesn't take into account the width of the signal,
    /// just the carrier center check
    for(unsigned int i=0;i<_tx_limits.size();i++)
    {
        std::complex<long long> band = _tx_limits.at(i);
        if(tx_freq > band.real() && tx_freq < band.imag())
            return true;
    }
    return false;
}
