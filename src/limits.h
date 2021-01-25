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

#ifndef LIMITS_H
#define LIMITS_H

#include <vector>
#include <complex>
#include <map>

class Limits
{
public:
    Limits();
    bool checkLimit(long tx_freq);

private:
    std::vector<std::complex<long>> _tx_limits;
    std::string _allocation_name;
};

#endif // LIMITS_H
