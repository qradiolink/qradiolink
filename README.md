QRadioLink
==========

About
-----
This application is a building platform which allows experimenting with VHF-UHF SDR transceivers using different modulation schemes for digital data transmissions.
So far digital voice and text transmission is supported, using either a narrow band modem and Codec2 or a high bandwidth modem and Opus.
Supported hardware includes the Ettus USRP, RTL-SDR, HackRF, BladeRF and in general all devices supported by libgnuradio-osmosdr
 

Requirements
------------
- All packages require binaries and development packages (headers)

- QT >= 4.8 (QT>=5.3 does not work on Debian Jessie, it may work on other distributions)
- qmake (used as qmake-qt4)
- Pulseaudio (native Alsa support is not fully implemented) 
- Gnuradio >= 3.7.10 built with OsmoSDR and UHD support (On Debian you can find it in jessie-backports)
- Boost 
- libgnuradio-osmosdr built with UHD, HackRF, BladeRF or LimeSDR support
- libgsm, libprotobuf, libopus, libpulse-simple, libpulse, libasound, libcodec2, libsqlite3
- optional protoc compiler for development work only (libprotoc 2.6.1)


Building the software
---------------------

After cloning the Github repository into a directory of your choice:
<pre>
cd qradiolink
mkdir build 
cd build/
qmake-qt4 ..
make
./qradiolink
</pre>

Known issues:
Build fails due to include error in /usr/include/gnuradio/qtgui/qtgui_types.h (Debian): manually edit the header to specify the correct directory for QWT
#include <qwt/qwt_color_map.h>
#include <qwt/qwt_scale_draw.h>

Please report any errors while building the software using the Github tickets page


Running
-------
Currently some configuration options for osmosdr sources and sinks are hardcoded and not yet exposed to the GUI. You may need to edit the code manually to configure your devices.
The files are located inside the gr/ directory. If you are familiar with Gnuradio they should be self explanatory.
Most common options include operating frequency, frequency correction (PPM), device access strings, TX power and RX gain, antenna settings etc.
By default the devices will operate in the 433 MHz ISM band.
If you own an UHD device and an RTL-SDR device you do not need to edit the code. Start the application using the command line and check for any errors.
First select the wideband checkbox to toggle between low bandwidth Codec2 and high bandwidth Opus
Then enable the TX and/or RX checkboxes depending on which devices you have installed. The frequency will be set at 434000000 Hz.
The Tune page allows fine tuning around the frequency, TX gain adjusting and monitors the incoming signal for RX

As qradiolink is very experimental and the development is in constant flux, it is only recommended for advanced users who are confortable with Gnuradio and building/editing the code


Running the code on Android devices for portable VHF-UHF SDR in amateur radio scope
-----------------------------------------------------------------------------------
The current master branch supports running the application on recent Android mobile phones or tablets.Requires some knowledge of Android internals.
You will need to have the device fully unlocked and root access. A SIM card is not necessary.
First install an Android application which can create a Linux chroot and an Android VNC viewer.
Install Debian Jessie in the chroot using a fresh SDcard. Configure it so you can run SSH and an X server.
Compile qradiolink for the armhf EABI and install all required packages on the phone. Gnuradio has optimizations in for ARM using the Volk kernels.
Note: a phone may not be able to provide enough power to your SDR peripheral, so you may require an external battery pack. Only the RTL-SDR is confirmed to be powered correctly by the phone through the USB OTG interface.
Please report back any successful deployments.


Credits and License
-------------------
QRadioLink is designed and developed by Adrian Musceac YO8RZZ, and it is licensed under the GNU General Public License version 3. It makes use of other code under compatible licenses, and the authors are credited in the source files.
Thanks to the Debian project and the Gnuradio projects which provide most of packages needed to install and run.
Codec2 is developed by David Rowe
Opus is developed by the Xiph foundation
QRadioLink formerly used to be a VoIP repeater controller application which was repurposed. As such, its internal architecture has severe issues at the moment and needs lots of additional work. As such, there is no claim that this software can be suitable for any purposes.