// Written by Adrian Musceac YO8RZZ at gmail dot com, started August 2013.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
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

#include "aprsstation.h"

AprsStation::AprsStation()
{
    callsign = "";
    adressee = "";
    via = "";
    message = "";
    symbol = "";
    payload = "";
    time_seen = 0;
    latitude = 0.0;
    longitude = 0.0;
}

QString AprsStation::getImage()
{
    //hardcoded symbol table
    if(symbol == ">")
        return "14_1";
    if(symbol == "<")
        return "12_1";
    if(symbol == "#")
        return "3_0";
    if(symbol == "&")
        return "6_0";
    if(symbol == "W")
        return "7_3";
    if(symbol == "=")
        return "13_1";
    if(symbol == "r")
        return "2_5";
    if(symbol == "$")
        return "4_0";
    if(symbol == "I")
        return "9_2";
    if(symbol == "Y")
        return "9_3";
    if(symbol == "y")
        return "9_5";
    if(symbol == "b")
        return "2_4";
    if(symbol == "h")
        return "8_4";
    if(symbol == "d")
        return "4_4";
    if(symbol == "f")
        return "6_4";
    if(symbol == "a")
        return "1_4";
    if(symbol == "p")
        return "0_3";
    if(symbol == "k")
        return "11_4";
    if(symbol == "R")
        return "2_3";
    if(symbol == "B")
        return "2_2";
    if(symbol == "n")
        return "14_4";
    if(symbol == ";")
        return "11_1";
    if(symbol == "l")
        return "12_4";
    if(symbol == "Q")
        return "1_3";
    if(symbol == "O")
        return "15_2";
    if(symbol == "s")
        return "3_5";
    if(symbol == "u")
        return "5_5";
    if(symbol == "v")
        return "6_5";
    if(symbol == "c")
        return "3_4";
    if(symbol == "A")
        return "1_2";
    if(symbol == "C")
        return "3_2";
    if(symbol == "H")
        return "8_2";
    if(symbol == "-")
        return "13_0";
    if(symbol == "?")
        return "15_1";
    if(symbol == "_")
        return "15_3";
    if(symbol == "*")
        return "10_0";
    if(symbol == "0")
        return "0_1";
    if(symbol == "!")
        return "1_0";
    if(symbol == "[")
        return "11_3";
    if(symbol == "(")
        return "8_0";
    if(symbol == ")")
        return "9_0";
    if(symbol == "`")
        return "0_4";
    if(symbol == "%")
        return "5_0";
    if(symbol == "/")
        return "15_0";
    if(symbol == "+")
        return "11_0";
    if(symbol == "'" || symbol=="^")
        return "7_0";

    return "13_6";

}
