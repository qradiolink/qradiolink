QRadioLink
==========

[![Build Status](https://travis-ci.org/kantooon/qradiolink.svg?branch=master)](https://travis-ci.org/kantooon/qradiolink)

About
-----

*QRadioLink* is a Linux SDR transceiver application with VOIP support, built on top of [Gnuradio](https://www.gnuradio.org/), 
made for hobbyists, tinkerers and radio enthusiasts,
which allows experimenting with software defined radio hardware using different digital and analog modes and 
an easy to use user interface.

Its primary purpose is educational, but it can also be customized for low power private and public data communications
on various frequency bands.
Can also be used as an amateur radio SDR transceiver for teaching students about radio communications.


Features
---

- Transmit and receive analog voice, digital voice, text messages, digital video, IP protocol.
- Analog and digital mode repeater - full duplex mode, no mixed mode repeater
- VOIP connection between two or more stations operating in simplex or semi-duplex mode
- Direct VOIP talk-around
- Radio forwarding over VOIP - forward analog or digital radio to the VOIP connection and viceversa
- TLS session encryption
- Mixed operation mode: transmit one mode and receive another
- Digital voice codecs: Codec2 700 bit/s, Codec2 1400 bit/s, Opus 9600 bit/s
- Narrow band digital voice mode with the [Codec2](http://rowetel.com/codec2.html) audio codec
- Wideband digital voice mode with the [Opus](https://xiph.org) audio codec
- Digital modulation:  **BPSK**, **DQPSK**, **2FSK**, **4FSK**
- Analog modulation: narrow FM (5 kHz), FM (10 kHz), Wide FM (broadcast, receive-only), AM, SSB
- CTCSS encoder and decoder for analog FM
- Video formats: JPEG
- Touch screen friendly interface
- Supported hardware: [**Ettus USRP**](https://ettus.com), [**RTL-SDR**](https://osmocom.org/projects/sdr/wiki/rtl-sdr), LimeSDR (through SoapySDR), HackRF, BladeRF, other devices 
supported by [**gr-osmosdr**](https://osmocom.org/projects/sdr/wiki/GrOsmoSDR)
 

Requirements
----
- Build dependencies on Debian Stretch: 

<pre>libasound2 (>= 1.0.16), libboost-program-options1.62.0, libboost-system1.62.0, 
libboost-thread1.62.0, libc6 (>= 2.15), libcodec2-0.4, libconfig++9v5, libgcc1 (>= 1:3.0), 
libgnuradio-analog3.7.10, libgnuradio-audio3.7.10, libgnuradio-blocks3.7.10, 
libgnuradio-digital3.7.10, libgnuradio-fec3.7.10, libgnuradio-filter3.7.10, 
libgnuradio-osmosdr0.1.4, libgnuradio-pmt3.7.10, libgnuradio-qtgui3.7.10, 
libgnuradio-runtime3.7.10, libgsm1 (>= 1.0.13), libjpeg62-turbo (>= 1.3.1), 
libopus0 (>= 1.1), libprotobuf10, libpulse0 (>= 0.99.1), libqt4-network (>= 4:4.5.3), 
libqt4-sql (>= 4:4.5.3), libqtcore4 (>= 4:4.8.0), libqtgui4 (>= 4:4.6.1),
 libstdc++6 (>= 5.2), gnuradio-dev, gr-osmosdr, libgsm1-dev, libprotobuf-dev,
 libopus-dev, libpulse-dev, libcodec2-dev, libasound2-dev, libjpeg62-turbo-dev,
 libconfig++-dev, qt4-qmake, libqt4-dev, libqwt5-qt4-dev, libqt4-sql-sqlite, qt4-dev-tools
</pre>

- Please make sure you have all the development packages installed before building QRadioLink

- QT >= 4.8 (QT 5 only works on Debian testing with GNUradio 3.7.11 packages )
- qmake (used as qmake-qt4)
- Pulseaudio (native Alsa support is not fully implemented) 
- Gnuradio >= 3.7.10 built with OsmoSDR and UHD support
- Boost 
- libgnuradio-osmosdr built with UHD, RTL-SDR, SoapySDR or HackRF support
- libgsm, libprotobuf, libopus, libpulse-simple, libpulse, libasound, libcodec2, libsqlite3, libjpeg,
- protoc compiler for development work only (libprotoc 2.6.1 or greater)

In order to build on Ubuntu 17.10 you have to install the following packages, assuming a full GNU Radio development environment is already installed.

<pre>
$ sudo apt install libconfig++-dev libgsm1-dev libprotobuf-dev libopus-dev libpulse-dev libasound2-dev libcodec2-dev libsqlite3-dev libjpeg-dev libprotoc-dev protobuf-compiler libqwt5-qt4-dev
</pre>

[Downloads](https://github.com/kantooon/qradiolink/releases "Downloads")
----

Debian Stretch (stable) x86_64 packages are provided via Travis CI automated builds
Please see the [Github releases page](https://github.com/kantooon/qradiolink/releases) for binary downloads.

Opensuse packages are available from [Opensuse build server](https://build.opensuse.org/package/show/hardware:sdr/qradiolink)
thanks to Martin Hauke.

Building the software from source
-----

The guide assumes you are using Debian Stetch.
- Clone the Github repository into a directory of your choice
- Compile the protobuf sources for your system
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
- Digital reception sometimes stops working after switching modes. Workaround: select RX mode before starting RX.
- FFT display has incorrect size (too small)
Workaround: switch to waterfall and back
- Segmentation fault when starting TX or RX modes. 
Check that that device settings are correct and you have clicked save in the configuration page.
- In low light, the automatic adjustment of ISO in the video camera can cause very long times to capture a frame.
Solution: use plenty of lighting for video.



Running
-------
- It is recommended to start the application using the command line and check for errors.
- At first run, see the Setup tab first and configure the application, then click Save before starting TX or RX from the main page, otherwise you may get a segmentation fault
- Setup options include RX and TX frequency correction (in PPM), device access strings, 
RX and TX antenna settings as a string and your callsign which will be sent at the start of the transmission.
- The configuration file is located in $HOME/.config/qradiolink.cfg
- By default the device will operate in the 433 MHz ISM band.
- Adjust TX gain in dB and RX sensitivity from the main page. If you are driving an external amplifier check the waveform for distorsion.
- The select inputs in the lower right corner toggle between different operating modes. Repeater mode requires both RX and TX to use the same mode.
- Enable the TX and/or RX buttons depending on whether you want only RX, only TX or both.
- The frequency can be adjusted from the main page either by using the dial widget or by entering it in the text box near it. 
- The Tune page allows fine tuning 5-5000 KHz around the center frequency with the slider, and monitoring the 
signal with the constellation sink and the RSSI display.
- Video page will display received video stream. Right now only a limited number of cameras are 
supported for TX, and ISO settings/ exposure of the camera in low light can cause too low capture framerate, which can't be transmitted properly.
- VOIP uses [umurmur](https://github.com/umurmur/umurmur) as a reflector. The available channels and the logged in stations are also listed on the page once you have connected to the server. The server IP/hostname will be saved on application exit. You can use QRadioLink as a pure VOIP client without using the radio by selecting "Use PTT for VOIP". You can also forward the digital or analog radio voice to the VOIP reflector. Any voice packets coming from the reflector will be transmitted directly after transcoding in this case. Currently full duplex audio from more than two VOIP clients at the same time is not fully supported.


Running the code on Android devices
-----------------------------------
- The current master branch supports running the application on recent Android mobile phones or tablets.
- You will need to have the device fully unlocked and root access. A SIM card is not necessary.
- First install an Android application which can create a Linux chroot and an Android VNC viewer or X server.
Install Linux in the chroot using a fresh SDcard. Configure it so you can run SSH and an X server.
Install all required packages on the phone and compile QRadioLink. In the future a ready built SD card image may be provided.
- Note: a phone may not be able to provide enough power to your SDR peripheral, so you may require an 
external battery pack in some cases.

[QRadioLink mobile phone SDR transceiver](https://www.youtube.com/watch?v=93nWWASt5a4)


Credits and License
-------------------
- QRadioLink is designed by Adrian Musceac YO8RZZ, and is released under an Open Source License,
 the GNU General Public License version 3.
- It makes use of other code under compatible licenses, and the authors are credited in the source files.
- The CFreqCtrl widget is Copyright 2010 Moe Wheatley.
- [Codec2](http://rowetel.com/codec2.html) is developed by David Rowe
- [Opus](https://xiph.org) is developed by the Xiph foundation
- [Gnuradio](https://www.gnuradio.org/)  is a free software development toolkit that provides signal processing
blocks to implement software-defined radios and signal-processing systems.

