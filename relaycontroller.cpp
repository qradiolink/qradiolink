// Written by Adrian Musceac YO8RZZ , started October 2019.
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

#include "relaycontroller.h"

RelayController::RelayController(QObject *parent) : QObject(parent)
{

    _ftdi_relay = 0;
    _ftdi_relay_enabled = false;
    _relay_mask = new unsigned char[1];
    memset(_relay_mask,0,1);

    if ((_ftdi_relay = ftdi_new()) == 0)
    {
        std::cerr <<  "Could not open FTDI context" << std::endl;
        return;
    }
    int f = ftdi_usb_open(_ftdi_relay, 0x0403, 0x6001);
    if (f < 0 && f != -5)
    {
        std::cerr << "Unable to open FTDI FT245R USB FIFO device 0x0403, 0x6001: " << f << " " << ftdi_get_error_string(_ftdi_relay) << std::endl;
        ftdi_free(_ftdi_relay);
        return;
    }
    std::cout << "Succesfully opened relay" << std::endl;

    ftdi_set_bitmode(_ftdi_relay, 0xFF, BITMODE_BITBANG);
    _ftdi_relay_enabled = true;
}

RelayController::~RelayController()
{
    if(_ftdi_relay_enabled)
    {
        ftdi_disable_bitbang(_ftdi_relay);
        ftdi_usb_close(_ftdi_relay);
        ftdi_free(_ftdi_relay);
    }
    delete[] _relay_mask;
}

int RelayController::enableRelay(int relay_number)
{
    if(!_ftdi_relay_enabled)
    {
        return 1;
    }
    if(relay_number > 8)
    {
        std::cerr << "Relay number not supported" << std::endl;
        return 0;
    }

    _relay_mask[0] |= 1 << relay_number;
    int ret = ftdi_write_data(_ftdi_relay, _relay_mask, 1);
    if (ret < 0)
    {
        std::cerr << "Enable failed for relay number " << relay_number << " " << _relay_mask[0] << " " << ftdi_get_error_string(_ftdi_relay) << std::endl;
        return 0;
    }
    return 1;
}

int RelayController::disableRelay(int relay_number)
{
    if(!_ftdi_relay_enabled)
    {
        return 1;
    }
    if(relay_number > 8)
    {
        std::cerr << "Relay number not supported" << std::endl;
        return 0;
    }

    _relay_mask[0] &= 0 << relay_number;
    int ret = ftdi_write_data(_ftdi_relay, _relay_mask, 1);
    if (ret < 0)
    {
        std::cerr << "Disable failed for relay number " << relay_number << " " << ftdi_get_error_string(_ftdi_relay) << std::endl;
        return 0;
    }
    return 1;
}
