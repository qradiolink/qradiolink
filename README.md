QRadioLink
==========

[![Build Status](https://travis-ci.org/kantooon/qradiolink.svg?branch=master)](https://travis-ci.org/kantooon/qradiolink)

About
-----
- QRadioLink is a GUI for a collection of Gnuradio modems, built for hobbyists, tinkerers and radio enthusiasts,
which allows experimenting with SDR hardware using different digital and analog modes
- Possible applications: digital walkie talkie, baby monitor, video camera streaming, IoT devices which are RF
enabled, radio LAN, robotics.
- Receives and transmits analog voice, digital voice, low resolution video, text, IP protocol.
- Somewhat touchscreen friendly user interface.
- Digital audio transmission uses either a narrow band modem and Codec2 or a high bandwidth modem and Opus.
- Modems: BPSK, QPSK, 2FSK, 4FSK
- Modes: narrow FM, SSB, digital voice, digital video, digital data
- Audio formats: Codec2 700B, Codec2 1400, Opus 10 kbit/s
- Video formats: JPEG, x264 (work in progress)
- Supported hardware: Ettus USRP, RTL-SDR, HackRF, BladeRF and in general all devices 
supported by libgnuradio-osmosdr
 

Requirements
------------
- Build deps: libasound2 (>= 1.0.16), libboost-program-options1.62.0, libboost-system1.62.0, 
libboost-thread1.62.0, libc6 (>= 2.15), libcodec2-0.4, libconfig++9v5, libgcc1 (>= 1:3.0), 
libgnuradio-analog3.7.10, libgnuradio-audio3.7.10, libgnuradio-blocks3.7.10, 
libgnuradio-digital3.7.10, libgnuradio-fec3.7.10, libgnuradio-filter3.7.10, 
libgnuradio-osmosdr0.1.4, libgnuradio-pmt3.7.10, libgnuradio-qtgui3.7.10, 
libgnuradio-runtime3.7.10, libgsm1 (>= 1.0.13), libjpeg62-turbo (>= 1.3.1), 
libopus0 (>= 1.1), libprotobuf10, libpulse0 (>= 0.99.1), libqt4-network (>= 4:4.5.3), 
libqt4-sql (>= 4:4.5.3), libqtcore4 (>= 4:4.8.0), libqtgui4 (>= 4:4.6.1),
 libstdc++6 (>= 5.2), gnuradio-dev, gr-osmosdr, libgsm1-dev, libprotobuf-dev,
 libopus-dev, libpulse-dev, libcodec2-dev, libasound2-dev, libjpeg62-turbo-dev,
 libconfig++-dev, qt4-qmake, libqt4-dev, libqt4-phonon, libqt4-sql-sqlite, qt4-dev-tools

- Please make sure you have the development packages installed before building QRadioLink

- QT >= 4.8 (QT>=5.3 does not work on Debian 8, it may work on other distributions)
- qmake (used as qmake-qt4)
- Pulseaudio (native Alsa support is not fully implemented) 
- Gnuradio >= 3.7.10 built with OsmoSDR and UHD support (On Debian 8 you can find it in jessie-backports)
- Boost 
- libgnuradio-osmosdr built with UHD, HackRF, BladeRF or LimeSDR support
- libgsm, libprotobuf, libopus, libpulse-simple, libpulse, libasound, libcodec2, libsqlite3, libjpeg,
- optional protoc compiler for development work only (libprotoc 2.6.1)

[Downloads](https://github.com/kantooon/qradiolink/releases "Downloads")
---------

Debian Stretch (stable) x86_64 packages are provided via Travis CI automated builds
Please see the [Github releases page](https://github.com/kantooon/qradiolink/releases)

Building the software
---------------------

The guide assumes you are using Debian Stetch.
- Clone the Github repository into a directory of your choice
- Compile the protobuf sources for your system
- Work around the Debian issue with the QWT include paths in Gnuradio
- Run qmake to generate the Makefile
- Run make (with the optional -j flag)

<pre>
cd qradiolink
mkdir build
cd ext/
protoc --cpp_out=. Mumble.proto
protoc --cpp_out=. QRadioLink.proto
cd ..
cd build/
qmake-qt4 ..
make
./qradiolink
</pre>

Known issues:
- Segmentation fault when starting TX or RX modes. 
Check that you have clicked save in the configuration page
and that device settings are correct.
- In low light, the automatic adjustment of ISO in the video camera can cause very long times to capture a frame.
Solution: use plenty of lighting.



Running
-------
- Start the application using the command line and check for any errors.
- Please see the Setup tab first and make sure to click Save before starting TX or RX modes, otherwise you may get a segmentation fault
- Setup options include RX and TX frequency correction (in PPM), device access strings, 
RX and TX antenna settings as a string and your callsign which will be sent at the start of the transmission.
- The configuration file is located in $HOME/.config/qradiolink.cfg
- By default the device will operate in the 433 MHz ISM band.
- Adjust TX gain in dB and RX sensitivity from the TX tab. If you are driving an external amplifier check the waveform for distorsion.
- The select input in the lower right corner toggles between different operating modes.
- Enable the TX and/or RX buttons depending on whether you want only RX, only TX or both.
- Adjust the frequency from the TX tab either by using the dial widget or by entering it in the text box. 
- The Tune page allows fine tuning 5-5000 KHz around the frequency with the slider, and monitoring the 
signal with the constellation sink or the spectrum display (FFT/waterfall).
- Video page will display received video stream. Right now only a limited number of cameras are 
supported for TX, and ISO settings/ exposure of the camera might cause too low a framerate, which can't be transmitted properly.


Running the code on Android devices
-----------------------------------
- The current master branch supports running the application on recent Android mobile phones or tablets.
- Requires some knowledge of Android internals.
- You will need to have the device fully unlocked and root access. A SIM card is not necessary.
- First install an Android application which can create a Linux chroot and an Android VNC viewer.
Install Debian Jessie in the chroot using a fresh SDcard. Configure it so you can run SSH and an X server.
Install all required packages on the phone and compile QRadioLink. In the future a ready built SD card 
image may be provided.
- Note: a phone may not be able to provide enough power to your SDR peripheral, so you may require an 
external battery pack. Only the RTL-SDR is confirmed to be powered correctly by the phone through the 
USB OTG interface.

[QRadioLink running on a mobile phone](https://www.youtube.com/watch?v=93nWWASt5a4)


Credits and License
-------------------
- QRadioLink is based on a fork of a RoIP software and was redesigned by Adrian Musceac YO8RZZ with some help from others,
and it is licensed under the GNU General Public License version 3.
- It makes use of other code under compatible licenses, and the authors are credited in the source files.
- The CFreqCtrl widget is Copyright 2010 Moe Wheatley.
- Codec2 is developed by David Rowe
- Opus is developed by the Xiph foundation
- Gnuradio https://www.gnuradio.org/
- There is no claim that this software can be suitable for any purposes.

