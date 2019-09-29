QRadioLink
==========

[![Build Status](https://travis-ci.org/kantooon/qradiolink.svg?branch=master)](https://travis-ci.org/kantooon/qradiolink)

[![Packaging status](https://repology.org/badge/vertical-allrepos/qradiolink.svg)](https://repology.org/project/qradiolink/versions)

About
-----

*QRadioLink* is a GNU/Linux software defined radio VOIP (radio over IP) transceiver application using Internet for communication, 
built on top of [GNU radio](https://www.gnuradio.org/), 
which allows experimenting with software defined radio hardware using different digital and analog radio signals and a Qt5 user interface.

Its primary purpose is educational, but it can also be customized for low power data communications on various ISM frequency bands.
It can also be used as a low power amateur radio SDR transceiver for demonstrating radio communications to children at schools.

The application was originally inspired from the [Codec2 GMSK modem](https://github.com/on1arf/gmsk) project by Kristoff Bonne.

[![Screenshot](http://qradiolink.org/images/qradiolink47.png)](http://qradiolink.org)


Features
---

- VOIP (Radio-over-IP) connection between two or more stations operating in simplex or semi-duplex mode
- Radio forwarding over VOIP - forward voice to the VOIP connection and viceversa
- Direct VOIP talk-around (only requires connection to a VOIP server and no radio)
- Transmit and receive analog FM, SSB, AM, digital voice, text messages, digital video, IP protocol.
- Full duplex 250 kbit/s IP radio modem with configurable TX/RX offsets
- Mixed operation mode (receive one mode and transmit another)
- Full duplex and simplex operation
- Memory channels and memory channel scan
- Split operation (transmit on other frequency than the receive frequency with no shift limitation, used mostly for repeater operation)
- Digital voice codecs: Codec2 700 bit/s, Codec2 1400 bit/s, Opus 9600 bit/s
- FreeDV digital voice modulator and demodulator (currently supports only 1600, 700C and 800XA modes)
- Digital modulations:  **BPSK**, **DQPSK**, **2FSK**, **4FSK**, **FreeDV 1600**, **FreeDV 700C**, **FreeDV 800XA**
- Analog modulations: FM (10 kHz), narrow FM (5 kHz), SSB, AM, Wide FM (broadcast, receive-only)
- CTCSS encoder and decoder for analog FM
- VOX mode
- Analog and digital mode repeater - full duplex mode, no mixed mode support
- Automatic carrier tracking and Doppler effect correction for all digital modes except FreeDV modes. The system can track Doppler shifts of 5-10 kHz, depending on mode. It requires a CNR of at least 10-12 dB, more for FSK modes than for PSK modes.
- Supported hardware: [**Ettus USRP bus devices**](https://ettus.com), [**RTL-SDR**](https://osmocom.org/projects/sdr/wiki/rtl-sdr), [**ADALM-Pluto (PlutoSDR)**](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/adalm-pluto.html), (supported with SoapySDR and [**SoapyPlutoSDR**](https://github.com/pothosware/SoapyPlutoSDR)), [**LimeSDR-mini**](https://www.crowdsupply.com/lime-micro/limesdr-mini) (partly supported, through SoapySDR), BladeRF, other devices supported by [**gr-osmosdr**](https://osmocom.org/projects/sdr/wiki/GrOsmoSDR) like HackRF and RedPitaya (not tested)
 

Requirements
----

- Since release 0.8.0-2, GNU radio version is changed to 3.7.13 and Qt5 is used for the graphical interface. Debian Buster is the base GNU/Linux distribution for which packages are built.
- Build dependencies on Debian Buster with Qt5 and GNU radio 3.7.13: 

<pre>
$ sudo apt-get install gnuradio-dev protobuf-compiler gr-osmosdr gnuradio libvolk1-dev libvolk1-bin libprotobuf17 libprotobuf-dev libopus0 libopus-dev libspeexdsp1 libspeexdsp-dev libpulse0 libpulse-dev libcodec2-0.8.1 libcodec2-dev libasound2 libasound2-dev libjpeg62-turbo libjpeg62-turbo-dev libconfig++9v5 libconfig++-dev qt5-qmake qt5-default qtbase5-dev libqt5core5a libqt5gui5 libqt5network5 libqt5sql5 libftdi1-dev libftdi1
</pre>

- Qt >= 5.11 and Qt5 development packages (older versions of Qt5 >= 5.2 might work as well)
- qmake
- Pulseaudio (native Alsa support is not fully implemented) 
- Gnuradio >= 3.7.13 built with UHD support and FreeDV/Codec2 support
- Boost and boost-devel
- libgnuradio-osmosdr (gr-osmosdr) built with UHD, RTL-SDR, SoapySDR, HackRF, RedPitaya or BladeRF support
- libprotobuf, libopus, libspeexdsp, libpulse-simple, libpulse, libasound, libcodec2, libsqlite3, libjpeg, libconfig++, libvolk, libftdi
- protoc compiler (libprotoc 2.6.1 or greater)
- optionally SoapySDR and SoapyPlutoSDR, SoapyLMS7

In order to build on Ubuntu 17.10 you have to install the following packages, assuming a full GNU Radio development environment is already installed.

<pre>
$ sudo apt install libconfig++-dev libprotobuf-dev libopus-dev libpulse-dev libasound2-dev libcodec2-dev libsqlite3-dev libjpeg-dev libprotoc-dev protobuf-compiler
</pre>

On Ubuntu 18.04 LTS, replace libjpeg62-turbo and libjpeg62-turbo-dev with libjpeg-turbo8-dev and libjpeg-dev
<pre>
$ sudo apt install libjpeg-turbo8-dev libjpeg-dev
</pre>

[Downloads](https://github.com/kantooon/qradiolink/releases "Downloads")
----

Debian Buster x86_64 packages are provided via Travis CI automated builds
Please see the [Github releases page](https://github.com/kantooon/qradiolink/releases) for binary downloads.

Opensuse packages are available from [Opensuse build server](https://build.opensuse.org/package/show/hardware:sdr/qradiolink)
thanks to Martin Hauke.

Building the software from source
-----

- Clone the Github repository into a directory of your choice
- Change directory to where you have cloned or unzipped the code
- Execute build_debian.sh

<pre>
$ sh ./build_debian.sh
</pre>

Or alternatively:

- Clone the Github repository into a directory of your choice
- Compile the protobuf sources for your system
- Run qmake to generate the Makefile
- Run make (with the optional -j flag)

<pre>
cd qradiolink
mkdir -p build
cd ext/
protoc --cpp_out=. Mumble.proto
protoc --cpp_out=. QRadioLink.proto
cd ../build/
qmake ..
make
./qradiolink
</pre>

Known issues:
- In low light, the automatic adjustment of ISO in the video camera can cause very long times to capture a frame. The solution is to use plenty of lighting for video.

Running
-------
- It is recommended to start the application using the command line when running the first few times.
- When first run, go to the Setup tab first and configure the options, then click Save before starting TX or RX.
- You can only transmit when you have selected a sample rate of 1 Msps (1000000). Other sample rates are for receiving only.
- VOIP uses [umurmur](https://github.com/umurmur/umurmur) as a server. A version known to work with qradiolink is mirrored at [qradiolink](https://github.com/qradiolink/umurmur)  You can use QRadioLink as a pure VOIP client without using the radio by selecting "Use PTT for VOIP". For radio over IP operation, you need to toggle "Forward radio" to send the digital or analog radio voice to the VOIP server. Any voice packets coming from the server will be transmitted directly after transcoding in this case. Currently full duplex audio from more than two VOIP clients at the same time is not supported.
- The configuration file is located in $HOME/.config/qradiolink.cfg
- In full duplex operation you need to have sufficient isolation between the TX antenna port and the RX antenna port to avoid overloading your input.
- Video will be displayed in the upper right corner. If your camera does not work, see the V4L2 guide in the docs/ directory for troubleshooting camera settings.
- IP over radio operation mode requires net administration priviledges to be granted to the application. See the instructions in the docs/ directory.
- VOX mode requires careful setup of system microphone gain to avoid getting stuck on transmit.


Credits and License
-------------------
- QRadioLink is written by Adrian Musceac YO8RZZ, and is released under an Open Source License,
 the GNU General Public License version 3.
- The CFreqCtrl and CPlotter widgets are Copyright 2010 Moe Wheatley and Alexandru Csete OZ9AEC.
- It makes use of other code under compatible licenses, and the authors are credited in the source files.
- [GNU radio](https://www.gnuradio.org/)  is a free software development toolkit that provides signal processing
blocks to implement software-defined radios and signal-processing systems.
- [Codec2](http://rowetel.com/codec2.html) is developed by David Rowe
- [Opus](https://xiph.org) is developed by the Xiph foundation

