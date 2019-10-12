QRadioLink
==========

[![Build Status](https://travis-ci.org/kantooon/qradiolink.svg?branch=master)](https://travis-ci.org/kantooon/qradiolink)

[![Packaging status](https://repology.org/badge/vertical-allrepos/qradiolink.svg)](https://repology.org/project/qradiolink/versions)

About
-----

*QRadioLink* is a VOIP (radio over IP) GNU/Linux software defined radio transceiver application using Internet protocols for communication, 
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
- Mixed operation mode (receive one mode and transmit another)
- Full duplex and simplex operation
- Memory channels (store frequency, name, TX shift, operating mode, squelch value, volume, TX power, RX gain, TX and RX CTCSS) and memory channel scan 
- Split operation (transmit on other frequency than the receive frequency with no shift limitation, used mostly for repeater operation)
- Digital voice codecs: Codec2 700 bit/s, Codec2 1400 bit/s, Opus 9600 bit/s
- FreeDV digital voice modulator and demodulator (currently supports only 1600, 700C and 800XA modes)
- Digital modulations: **FreeDV 1600**, **FreeDV 700C**, **FreeDV 800XA**, **BPSK**, **DQPSK**, **2FSK**, **4FSK**
- Digital voice modem bitrates over the air from 1 kbit/s to 10 kbit/s
- Video bitrate below 250 kbit/s at 10 frames/second (currently no sound with video)
- Analog modulations: FM (12.5 kHz), narrow FM (6.25 kHz), SSB, AM, Wide FM (broadcast, receive-only)
- CTCSS encoder and decoder for analog FM
- VOX mode
- Analog and digital mode repeater - full duplex mode, no mixed mode support (yet)
- Full duplex 250 kbit/s IP radio modem with configurable TX/RX offsets
- Automatic carrier tracking and Doppler effect correction for all digital modes except FreeDV modes. The system can track Doppler shifts of 5-10 kHz, depending on mode. It requires a CNR of at least 10-12 dB, more for FSK modes than for PSK modes.
- Supported hardware: [**Ettus USRP bus devices**](https://ettus.com), [**RTL-SDR**](https://osmocom.org/projects/sdr/wiki/rtl-sdr), [**ADALM-Pluto (PlutoSDR)**](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/adalm-pluto.html), (supported with SoapySDR and [**SoapyPlutoSDR**](https://github.com/pothosware/SoapyPlutoSDR)), [**LimeSDR-mini**](https://www.crowdsupply.com/lime-micro/limesdr-mini) (partly supported, through SoapySDR), BladeRF, other devices supported by [**gr-osmosdr**](https://osmocom.org/projects/sdr/wiki/GrOsmoSDR) like HackRF and RedPitaya (not tested)
 

Requirements
----

- Since release 0.8.0-2, GNU radio version is changed to 3.7.13 and Qt5 is used for the graphical interface. Debian Buster is the base GNU/Linux distribution for which packages are built.
- Build dependencies on Debian Buster with Qt5 and GNU radio 3.7.13: 

<pre>
$ sudo apt-get install gnuradio-dev protobuf-compiler gr-osmosdr gnuradio libvolk1-dev libvolk1-bin libprotobuf17 libprotobuf-dev libopus0 libopus-dev libspeexdsp1 libspeexdsp-dev libpulse0 libpulse-dev libcodec2-0.8.1 libcodec2-dev libasound2 libasound2-dev libjpeg62-turbo libjpeg62-turbo-dev libconfig++9v5 libconfig++-dev qt5-qmake qt5-default qtbase5-dev libqt5core5a libqt5gui5 libqt5network5 libqt5sql5 qtmultimedia5-dev libqt5multimediawidgets5 libqt5multimedia5-plugins libqt5multimedia5 libftdi1-dev libftdi1
</pre>

- Qt >= 5.11 and Qt5 development packages (older versions of Qt5 >= 5.2 might work as well)
- qmake
- Pulseaudio or Alsa or Jack
- Gnuradio >= 3.7.13 built with UHD support and FreeDV/Codec2 support
- Boost and boost-devel
- libgnuradio-osmosdr (gr-osmosdr) built with UHD, RTL-SDR, SoapySDR, HackRF, RedPitaya or BladeRF support
- libprotobuf, libopus, libspeexdsp, libpulse-simple, libpulse, libasound, libcodec2, libsqlite3, libjpeg, libconfig++, libvolk, libftdi, qtmultimedia5-dev, libqt5multimediawidgets5, libqt5multimedia5, libqt5multimedia5
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
$ git clone https://github.com/kantooon/qradiolink
$ cd qradiolink/
$ sh ./build_debian.sh
</pre>

Or alternatively:

- Clone the Github repository into a directory of your choice
- Compile the protobuf sources for your system
- Run qmake to generate the Makefile
- Run make (with the optional -j flag)

<pre>
git clone https://github.com/kantooon/qradiolink
cd qradiolink/
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
- In low light, the automatic adjustment of ISO in the video camera can cause very long times to capture a frame. The solution is to use plenty of lighting for video. Otherwise the video transmission will experience very frequent interruptions.


Setup and running
-------
- It is recommended to start the application using the command line when running the first few times and look for any error messages output to the console. Some of them can be ignored safely, others are critical. Logging to a file is not yet enabled.
- When first run, go to the **Setup** tab first and configure the options, then click Save before starting TX or RX. Without the correct device arguments, the application can crash when enabling RX or TX. This is not something that the application can control and keep functioning properly.
- GNU radio main DSP blocks are highly optimized (including on embedded ARM platforms) by using the VOLK library. To minimize the CPU resources consumed by QRadioLink it is recommended to run the **volk_profile** utility after GNU radio has been installed. This command only needs to be run when GNU radio or libvolk are upgraded.
- High sample rates, high FPS rates and high FFT sizes all affect the CPU performance adversely. On embedded platforms with low resources, you can disable the spectrum display completely using the FFT checkbox. The FPS value also sets the rate at which the S-meter and constellation display are updated, so reduce it to minimum usable values. If the controls menu is not visible, the S-meter display will not consume CPU resources. Similar for the Constellation display.
- You can only transmit when you have selected a sample rate of 1 Msps (1000000). Other sample rates are for receiving only (except if you are using two different devices for receive and transmit). This is a hardware limitation on most devices because the transmit sample rate is fixed at 1 Msps to save CPU resources and most hardware cannot cope with two sample rates simultaneously.
- Filter widths for reception and transmission of the analog modes (FM, SSB, AM) are configurable. To increase or decrease them, drag the margins of the filter box on the spectrum display.
- VOIP uses [umurmur](https://github.com/umurmur/umurmur) as a server. A version known to work with qradiolink is mirrored at [qradiolink](https://github.com/qradiolink/umurmur)  You can use QRadioLink as a pure VOIP client without using the radio by selecting "Use PTT for VOIP". For radio over IP operation, you need to toggle "Forward radio" to send the digital or analog radio voice to the VOIP server and viceversa. Any voice packets coming from the server will be transmitted directly after transcoding in this case. Currently full duplex audio from more than one VOIP client at the same time is not supported. The **Mumble** application can now receive and talk to QRadioLink normally. You should enable **Push To Talk** in Mumble and maximize the network robustness settings. Text messages from Mumble are displayed inside the application, but no action is taken on them (yet). Text messages can also be sent to the current Mumble channel.
The Mumble VOIP connection uses the Opus codec at a higher bitrate, so ensure the server can handle bitrates up to 50 kbit/s per client.
- The VOIP username will be your callsign, plus a number of 4 random characters allowing you to use multiple clients on the same server. VOIP password is not yet supported.
- The configuration file is located in $HOME/.config/qradiolink/qradiolink.cfg
- The memory channels storage file is located in $HOME/.config/qradiolink/qradiolink_mem.cfg
- After adding a memory channel, you can edit its values by double clicking on a table cell. This may cause the radio to switch to that channel. The settings are not updated instantly, so if you make a change, after you press Enter, switch to another channel and back to get the updates. A button allows you to save channels before the window is closed.  Saving sorted channels is not possible yet. Otherwise, the channels, like the settings, will be stored on exit (if no application crash meanwhile).
- **Before any upgrade**, please make a backup of the $HOME/.config/qradiolink/ directory in case something goes wrong, to avoid losing settings and channels.
- Baseband gain can be safely ignored on most devices. It was added as a workaround for the PlutoSDR and is no longer required. Leave it at 1 unless you know better.
- In full duplex operation you need to have sufficient isolation between the TX antenna port and the RX antenna port to avoid overloading your input or destroying the LNA stage.
- In half duplex mode the receiver is muted during transmit and the RX gain is minimized. Do not rely on this feature if using a power amplifier, please use a RF switch (antenna switch) with enough isolation, or introduce attenuators in the relay sequence to avoid destroying the receiver LNA.
- The transmitter of the device is active at all times if enabled, even when no samples are being output. Although there is no signal being generated, local oscillator leakage may be present and show up on the spectrum display. This is not a problem usually, unless if you keep a power amplifier connected and enabled at all times. You can use the USB relays to disable it in this case when not transmitting.
- FreeDV modes and PSK modes are very sensitive to amplifier non-linearity. You should not try to use them within a non-linear envelope to avoid signal distortion, splatter or unwanted spectrum components. Digital gain for these modes has been set in such a way to avoid non-linear zone for most devices output stages. If this is not satisfactory, you can use the baseband gain setting to increase the digital gain.
- Receive and transmit gains currently operate as described in the gr-osmosdr manual. At lowest settings, the programmable gain attenuator will be set, following with any IF stages if present and finally any LNA stages if present. This behaviour is desirable since there is no point setting the LNA to a higher value than the PGA if the signal power is already above the P1dB point of the LNA stage. While this behaviour is simple and easy to understand, in the future it may be possible to adjust all gain stages separately.
- Transmit shift can be positive or negative. After changing the value, you need to press **Enter** to put it into effect. Setting the TX shift is not possible while transmitting a signal. Although the shift is stored as Hertz and you can edit this value in the config, the UI will only allow a value in kHz to be entered (e.g. -7600 kHz standard EU UHF repeater shift).
- USB relays using FTDI chipsets are used to control RF switches, power amplifiers and filter boards. To determine if your USB relay board is supported, look for a similar line in  the output of lsusb:
<pre>
Bus 002 Device 003: ID 0403:6001 Future Technology Devices International, Ltd FT232 Serial (UART) IC
</pre>
Do note that the identifier digits are the most important: **0403:6001**
- QRadioLink can control a maximum of 8 relays, however only 2 are used at the moment. This is work in progress. Other types of relays may be supported in the future.
- Video will be displayed in the upper right corner. If your camera does not work, see the V4L2 guide in the docs/ directory for troubleshooting camera settings.
- IP over radio operation mode requires net administration priviledges to be granted to the application. See the instructions in the docs/ directory. An error message will be output at startup if these priviledges are not present. You can safely ignore this message if you don't need to use the IP modem facility.
- VOX mode requires careful setup of system microphone gain to avoid getting stuck on transmit. The voice activation system is not very robust right now and may be improved in the future.
- Setting application internal microphone gain above the middle of the scale might cause clipping and distortion of audio, as the system volume also affects what goes to the radio.
- The S-meter calibration feature is not complete yet, however you can enter in the Setup tab the level (integer value expressed in dBm) of a known signal (e.g. sent by a generator) to correct the reading. Do NOT apply signals with levels above -30 to 0 dBm to the receiver input as this might damage your receiver, depending on hardware. Please note that the RSSI and S-meter values displayed are relative to the current operating mode filter bandwidth, so the FM reading will be different to a SSB reading! Calibration tables support for different bands may be provided in the future.
- The network control feature (for headless mode) is not complete yet. For testing purposes, the control port is 4939 (apologies if I stepped onto some IANA assignment). To test and use this feature, you can simply use telnet from the same computer:
<pre>
$ telnet localhost 4939
Trying ::1...
Connected to localhost.
Escape character is '^]'.
Welcome! Available commands are: 
rxstatus
txstatus
txactive
rxstatus
RX status is inactive.
rxstatus
RX status is active.
txstatus
TX status is inactive.
txstatus
TX status is active.
txactive
Not transmitting.
txactive
Currently transmitting.
Server is stopping now.
Connection closed by foreign host.
</pre>



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

