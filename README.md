QRadioLink
==========

[![Build Status](https://travis-ci.org/qradiolink/qradiolink.svg?branch=master)](https://travis-ci.org/qradiolink/qradiolink)

[![Packaging status](https://repology.org/badge/vertical-allrepos/qradiolink.svg)](https://repology.org/project/qradiolink/versions)

About
-----

*QRadioLink* is a VOIP (radio over IP) GNU/Linux software defined radio transceiver application using Internet protocols for communication, 
built on top of [GNU radio](https://www.gnuradio.org/), 
which allows experimenting with software defined radio hardware using different digital and analog radio signals and a Qt5 user interface.

Its primary purpose is educational, but it can also be customized for low power data communications on various ISM frequency bands.
It can also be used as a low power amateur radio SDR transceiver for demonstrating radio communications to children at schools.

The application was originally inspired from the [Codec2 GMSK modem](https://github.com/on1arf/gmsk) project by Kristoff Bonne.

[![Screenshot](http://qradiolink.org/images/qradiolink51.png)](http://qradiolink.org)


Alternatives to QRadioLink
---

Free software projects that work on Linux and have similar features to QRadioLink are listed below.

- [**FreeDV**](https://freedv.org/) is a Digital Voice mode for HF radio. The application works for Windows, Linux and OSX and allows any SSB radio to be used for low bit rate digital voice. It is the original free software Codec2 implementation. It does not require a SDR and works with any analog radio.
- [**SvxLink**](https://www.svxlink.org/) is a great project which inspired the radio linking features of QRadioLink. The Qtel component is a full-featured Echolink GUI client. It does not require using a SDR and can work with any FM radio.
- [**SDRangel**](https://github.com/f4exb/sdrangel) is a full SDR transceiver for SSB, FM, DMR, D-Star, C4FM and DVB-S. It can use only SDR hardware but it supports a large number of them.
- [**Mumble**](https://www.mumble.info/) is what QRadioLink uses under the hood. It is a great alternative for people who don't want to use SDR radios.
- [**Codec2 GMSK**](https://github.com/on1arf/gmsk) is a great and free software alternative to D-Star on VHF-UHF handheld radios. It only requires a radio capable of 9600 baud packet.
- [**OP25**](http://osmocom.org/projects/op25/wiki) is a free software implementation of D-Star, DMR and C4FM (Yaesu digital voice standard). It works with FM radios capable of 9600 baud packet as well as SDRs.
- [**Charon**](https://github.com/tvelliott/charon) is a stand-alone OFDM transceiver with batman-adv mesh networking capabilities. The IP modem in Charon is very advanced and can be embedded on the PlutoSDR. It is the base for several amateur radio mesh networks. Only works with SDR hardware.
- [**MMDVM**](https://github.com/g4klx/MMDVM) extremely robust free software implementation of D-Star, DMR and C4FM (Yaesu digital voice standard). Works with RaspberryPi, Arduino and any radios capable of 9600 baud packet.


Features
---

- VOIP (Radio-over-IP) connection between two or more stations operating in simplex or semi-duplex mode
- Radio forwarding over VOIP - forward voice to the VOIP connection and viceversa
- Direct VOIP talk-around (only requires connection to a VOIP server and no radio)
- Wideband digital voice streaming over the Internet with the **Opus** audio codec
- Remote control via network (requires a telnet client or similar program, can be scripted)
- Remote control via Mumble private text messages
- Run headless (no graphical user interface) for terminal usage on embedded platforms like the Raspberry Pi or similar boards without any screen
- Night mode theme
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
- Configurable filter widths for analog modes
- CTCSS encoder and decoder for analog FM
- VOX mode
- Analog and digital mode repeater - in full duplex mode only, same mode or mixed mode repeater (e.g. FM to Codec2 and viceversa, or FM to Opus and viceversa)
- Repeater linking via VOIP and Mumble - a group of repeaters can be linked duplex by sharing the same Mumble channel. This feature is still experimental and WIP.
- Internal audio mixing of audio from VOIP server and audio from radio
- USB FTDI (FT232) relay control support (for RF switches, power amplifier and filter control)
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
- Gnuradio >= 3.7.13 built with UHD, SoapySDR support and FreeDV/Codec2 support. Gnuradio 3.8 is not supported yet.
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

[Downloads](https://github.com/qradiolink/qradiolink/releases "Downloads")
----

Debian Buster x86_64 packages are provided via Travis CI automated builds
Please see the [Github releases page](https://github.com/qradiolink/qradiolink/releases) for binary downloads.

Opensuse packages are available from [Opensuse build server](https://build.opensuse.org/package/show/hardware:sdr/qradiolink)
thanks to Martin Hauke.

Building the software from source
-----

- Clone the Github repository into a directory of your choice
- Change directory to where you have cloned or unzipped the code
- Execute build_debian.sh

<pre>
$ git clone https://github.com/qradiolink/qradiolink
$ cd qradiolink/
$ git checkout master
$ sh ./build_debian.sh
</pre>

Or alternatively:

- Clone the Github repository into a directory of your choice
- Compile the protobuf sources for your system
- Run qmake to generate the Makefile
- Run make (with the optional -j flag)

<pre>
git clone https://github.com/qradiolink/qradiolink
cd qradiolink/
git checkout master
mkdir -p build
cd ext/
protoc --cpp_out=. Mumble.proto
protoc --cpp_out=. QRadioLink.proto
cd ../build/
qmake ..
make
./qradiolink
</pre>

You can add custom library search paths, custom includes paths and specify a different install directory using these variables with qmake:
<pre>
qmake .. INSTALL_PREFIX=/usr/local/bin LIBDIR=/opt/lib INCDIR=/opt/include
</pre>

Known issues:
- In low light, the automatic adjustment of ISO in the video camera can cause very long times to capture a frame. The solution is to use plenty of lighting for video. Otherwise the video transmission will experience very frequent interruptions.


Setup and running
-------
- It is recommended to start the application using the command line when running the first few times and look for any error messages output to the console. Some of them can be ignored safely, others are critical. Logging to console is by default enabled.
- **It is not recommended to run qradiolink as root**
- When first run, go to the **Setup** tab first and configure the options, then click Save before starting TX or RX. Without the correct device arguments, the application can crash when enabling RX or TX. This is not something that the application can control and keep functioning properly.
- GNU radio main DSP blocks are highly optimized (including on embedded ARM platforms) by using the VOLK library. To minimize the CPU resources consumed by QRadioLink it is recommended to run the **volk_profile** utility after GNU radio has been installed. This command only needs to be run when GNU radio or libvolk are upgraded.
- High sample rates, high FPS rates and high FFT sizes all affect the CPU performance adversely. On embedded platforms with low resources, you can disable the spectrum display completely using the FFT checkbox. The FPS value also sets the rate at which the S-meter and constellation display are updated, so reduce it to minimum usable values. If the controls menu is not visible, the S-meter display will not consume CPU resources. Similar for the Constellation display.
- Pulseaudio can be configured for low latency audio by changing settings in /etc/pulse. Alsa may require you to place an **.asoundrc** file in the home directory with contents similar to this:
<pre>
period_time 0
period_size 1024
buffer_size 4096
rate 48000
</pre>
- You can only transmit when you have selected a sample rate of 1 Msps (1000000). Other sample rates are for receiving only (except if you are using two different devices for receive and transmit). This is a hardware limitation on most devices because the transmit sample rate is fixed at 1 Msps to save CPU resources and most hardware cannot cope with two sample rates simultaneously.
- Filter widths for reception and transmission of the analog modes (FM, SSB, AM) are configurable. To increase or decrease them, drag the margins of the filter box on the spectrum display.
- VOIP uses [umurmur](https://github.com/umurmur/umurmur) as a server. A version known to work with qradiolink is mirrored at [qradiolink](https://github.com/qradiolink/umurmur)  You can use QRadioLink as a pure VOIP client without using the radio by selecting "Use PTT for VOIP". For radio over IP operation, you need to toggle "Forward radio" to send the digital or analog radio voice to the VOIP server and viceversa. Any voice packets coming from the server will be transmitted directly after transcoding in this case. Full duplex audio from more than one VOIP client at the same time can now be transmitted. The **Mumble** application is now also compatible with QRadioLink. It is recommended to enable **Push To Talk** in Mumble and maximize the network robustness and latency settings. Text messages from Mumble are displayed inside the application, but no action is taken for channel-wide messages. Text messages can also be sent to the current Mumble channel. If remote control is enabled, private Mumble text messages will control the radio.
The Mumble VOIP connection uses the Opus codec at a higher bitrate, so ensure the server can handle bitrates up to 50 kbit/s per client.
- The VOIP username will be your callsign, plus a number of 4 random characters allowing you to use multiple clients on the same server. The server password is stored in plain text inside the config file. You can use chmod to set this file readable by your user only.
- Remote control via Mumble private text messages requires enabling remote control in settings, and using the Mumble client to send text messages to the QRadioLink username. Text messages sent to the channel will be ignored by the application. Authentication of user who is sending the commands is not yet implemented.
- Running headless (no graphical user interface) for usage on embedded platforms like the Raspberry Pi or similar boards requires starting QRadioLink from the command line with the **--headless** option; example:
<pre>
$ qradiolink --headless  >> $HOME/.config/qradiolink/qradiolink.log 2>&1
</pre>
When running in headless mode, console log will be disabled by default with the above command. Init scripts for SysV/systemd will be provided at some point to be able to run QRadioLink as a system service. When running headless from CLI, the network command server is started by default listening on the port configured in the settings file (or 4939 if not configured). Headless and remote operation will usually require you to enable VOIP forwarding either in the configuration file or via a command, unless you want to use audio from the machine where QRadioLink is running. CPU consumption can reach 50% at 800 MHz CPU clock for a headless QRadioLink instance connected to the VOIP network and operating as a duplex repeater (depending on mode used).
- The configuration file is located in $HOME/.config/qradiolink/qradiolink.cfg
- The memory channels storage file is located in $HOME/.config/qradiolink/qradiolink_mem.cfg
- Log messages are stored in $HOME/.config/qradiolink/qradiolink.log (this location will likely change in the future)
- After adding a memory channel, you can edit its values by double clicking on a table cell. This may cause the radio to switch to that channel. The settings are not updated instantly, so if you make a change, after you press Enter, switch to another channel and back to get the updates. A button allows you to save channels before the window is closed.  Saving sorted channels is not possible yet. Otherwise, the channels, like the settings, will be stored on exit (if no application crash meanwhile).
- **Before any upgrade**, please make a backup of the $HOME/.config/qradiolink/ directory in case something goes wrong, to avoid losing settings and channels.
- Digital gain can be safely ignored on most devices. It was added as a workaround for the PlutoSDR and is no longer required. Leave it at 1 unless you know better.
- In full duplex operation you need to have sufficient isolation between the TX antenna port and the RX antenna port to avoid overloading your input or destroying the LNA stage.
- In half duplex mode the receiver is muted during transmit and the RX gain is minimized. Do not rely on this feature if using a power amplifier, please use a RF switch (antenna switch) with enough isolation, or introduce attenuators in the relay sequence to avoid destroying the receiver LNA.
- The transmitter of the device is active at all times if enabled, even when no samples are being output. Although there is no signal being generated, local oscillator leakage may be present and show up on the spectrum display. This is not a problem usually, unless if you keep a power amplifier connected and enabled at all times. You can use the USB relays to disable it in this case when not transmitting.
- FreeDV modes and PSK modes are very sensitive to amplifier non-linearity. You should not try to use them within a non-linear envelope to avoid signal distortion, splatter or unwanted spectrum components. Digital gain for these modes has been set in such a way to avoid non-linear zone for most devices output stages. If this is not satisfactory, you can use the digital gain setting to increase the digital gain.
- Receive and transmit gains currently operate as described in the gr-osmosdr manual. At lowest settings, the programmable gain attenuator will be set, following with any IF stages if present and finally any LNA stages if present. This behaviour is desirable since there is no point setting the LNA to a higher value than the PGA if the signal power is already above the P1dB point of the LNA stage. While this behaviour is simple and easy to understand, in the future it may be possible to adjust all gain stages separately.
- Transmit shift can be positive or negative. After changing the value, you need to press **Enter** to put it into effect. Setting the TX shift is not possible while transmitting a signal. Although the shift is stored as Hertz and you can edit this value in the config, the UI will only allow a value in kHz to be entered (e.g. -7600 kHz standard EU UHF repeater shift).
- USB relays using FTDI (FT232) chipsets are used to control RF switches, power amplifiers and filter boards. To determine if your USB relay board is supported, look for a similar line in  the output of lsusb:
<pre>
Bus 002 Device 003: ID 0403:6001 Future Technology Devices International, Ltd FT232 Serial (UART) IC
</pre>
Do note that the identifier digits are the most important: **0403:6001**
- QRadioLink can control a maximum of 8 relays, however only 4 are used at the moment. This is work in progress. Other types of relays may be supported in the future. The order in which relays are activated and deactivated during a transmission cycle is as follows: activation starting with relay 1 to relay 8, deactivation in reverse order (relay 8 to relay 1). A Python script **( ext/ftdi.py )** is included to help you determine the order of relays on the board.
- Video will be displayed in the upper right corner. If your camera does not work, see the V4L2 guide in the docs/ directory for troubleshooting camera settings.
- IP over radio operation mode requires net administration priviledges to be granted to the application. See the instructions in the docs/ directory. An error message will be output at startup if these priviledges are not present. You can safely ignore this message if you don't need to use the IP modem facility.
- VOX mode requires careful setup of system microphone gain to avoid getting stuck on transmit. The voice activation system is not very robust right now and may be improved in the future.
- Repeater mode requires the radio to operate in **Duplex** mode. Prior to enabling repeater mode, make sure to configure the TX shift (positive or negative). Mixed mode repeat is  possible, so you can operate the receiver on a different mode to the transmitter (FM to Codec2/Opus/FreeDV or viceversa). If radio forwarding is enabled, audio from the repeater will be broadcast to the VOIP network as well. The repeater can now handle mixing of audio incoming from the VOIP network and coming from the radio receiver so it is possible for two or more users on different connected repeaters to speak simultaneously.
- When operating a repeater linked to the VOIP network, you may experience small delays of voice due to transcoding operations, especially for mixed mode repeaters.
- Setting application internal microphone gain above the middle of the scale might cause clipping and distortion of audio, as the system volume also affects what goes to the radio.
- The VOIP volume slider controls the volume of the audio **sent** to the Mumble server.
- It is now possible to mute self or deafen self from the UI without disconnecting from the VOIP server.
- The S-meter calibration feature is not complete yet, however you can enter in the Setup tab the level (integer value expressed in dBm) of a known signal (e.g. sent by a generator) to correct the reading. Do NOT apply signals with levels above -30 to 0 dBm to the receiver input as this might damage your receiver, depending on hardware. Please note that the RSSI and S-meter values displayed are relative to the current operating mode filter bandwidth, so the FM reading will be different to a SSB reading! Calibration tables support for different bands may be provided in the future.
- The network remote control feature (for headless mode) is work in progress. The network server will listen on all network interfaces and the default control port is 4939. There is no provision for authentication of the user, if you need security you can filter the remote control port in the firewall, use SSH to log in to the remote system and telnet from there to localhost port 4939. To use the network remote control feature, you can simply use the telnet program or you can create simple Python or shell scripts to automate the commands. The help command will list all the available commands as well as parameters:
<pre>
$ telnet localhost 4939
Trying ::1...
Connected to localhost.
Escape character is '^]'.
qradiolink> rxstatus
RX status is active.
qradiolink> quit
Bye!
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
- [Mumble](https://wiki.mumble.info/wiki/Main_Page) is an open source, low-latency, high quality voice chat software primarily intended for use while gaming. Various third party applications and libraries exist for varying use cases, like web interfaces for server administration, user- and channel-viewers, bots like music bots and more.

