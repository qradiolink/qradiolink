QRadioLink
==========

About
-----
- This application is a building platform based on Gnuradio which allows experimenting with 
VHF-UHF SDR transceivers using different modulation schemes for digital data transmissions.
- Analog voice, digital voice, low resolution video and text transmission are supported.
- Digital audio transmission uses either a narrow band modem and Codec2 or a high bandwidth modem and Opus.
- Modes: BPSK, QPSK, 4FSK, FM
- Supported hardware includes the Ettus USRP, RTL-SDR, HackRF, BladeRF and in general all devices 
supported by libgnuradio-osmosdr
 

Requirements
------------
- All packages require binaries and development packages (headers)
- Please make sure you have the development packages installed before building QRadioLink

- QT >= 4.8 (QT>=5.3 does not work on Debian 8, it may work on other distributions)
- qmake (used as qmake-qt4)
- Pulseaudio (native Alsa support is not fully implemented) 
- Gnuradio >= 3.7.10 built with OsmoSDR and UHD support (On Debian 8 you can find it in jessie-backports)
- Boost 
- libgnuradio-osmosdr built with UHD, HackRF, BladeRF or LimeSDR support
- libgsm, libprotobuf, libopus, libpulse-simple, libpulse, libasound, libcodec2, libsqlite3, libjpeg, libphonon
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
- Build fails due to include error in /usr/include/gnuradio/qtgui/qtgui_types.h (Debian): 
manually edit the header to specify the correct directory for QWT

- #include <qwt/qwt_color_map.h>
- #include <qwt/qwt_scale_draw.h>



Running
-------
- Start the application using the command line and check for any errors.
- Please see the Setup tab first
- Setup options include RX and TX frequency correction (in PPM), device access strings, 
RX and TX antenna settings as a string and your callsign which will be sent at the start of the transmission.
- By default the device will operate in the 433 MHz ISM band.
- Adjust TX gain and RX sensitivity from the TX tab.
- Adjust the frequency from the TX tab either by using the dial widget or by entering it in the text box. 
- The select input in the lower right corner toggles between different operating modes.
- Enable the TX and/or RX buttons depending on whether you want only RX, only TX or both. 
- The Tune page allows fine tuning 5KHz around the frequency with the slider, and monitoring the 
signal with the constellation or the spectrum display
- Video page will display received video stream. Right now only a limited amount of cameras are 
supported for TX, and ISO settings/ exposure of the camera might cause too low a framerate, which can't be received.


Running the code on Android devices for portable VHF-UHF SDR in amateur radio scope
-----------------------------------------------------------------------------------
The current master branch supports running the application on recent Android mobile phones or tablets [1].
Requires some knowledge of Android internals.
You will need to have the device fully unlocked and root access. A SIM card is not necessary.
First install an Android application which can create a Linux chroot and an Android VNC viewer.
Install Debian Jessie in the chroot using a fresh SDcard. Configure it so you can run SSH and an X server.
Install all required packages on the phone and compile QRadioLink. In the future a ready built SD card 
image may be provided.
Note: a phone may not be able to provide enough power to your SDR peripheral, so you may require an 
external battery pack. Only the RTL-SDR is confirmed to be powered correctly by the phone through the 
USB OTG interface.

[1] see https://www.youtube.com/watch?v=93nWWASt5a4


Credits and License
-------------------
- QRadioLink is designed and developed by Adrian Musceac YO8RZZ, and it is licensed under the 
GNU General Public License version 3.
- It makes use of other code under compatible licenses, and the authors are credited in the source files.
- The CFreqCtrl widget is Copyright 2010 Moe Wheatley.
- Codec2 is developed by David Rowe
- Opus is developed by the Xiph foundation
- Gnuradio https://www.gnuradio.org/
- There is no claim that this software can be suitable for any purposes.

