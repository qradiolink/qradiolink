README
==========

[![Packaging status](https://repology.org/badge/vertical-allrepos/qradiolink.svg)](https://repology.org/project/qradiolink/versions)

Supported hardware
---

-  [**Ettus USRP bus devices**](https://ettus.com), [**RTL-SDR**](https://osmocom.org/projects/sdr/wiki/rtl-sdr), [**ADALM-Pluto (PlutoSDR)**](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/adalm-pluto.html), (supported with SoapySDR and [**SoapyPlutoSDR**](https://github.com/pothosware/SoapyPlutoSDR)), [**LimeSDR-mini**](https://www.crowdsupply.com/lime-micro/limesdr-mini), [**LimeNET-Micro**](https://wiki.myriadrf.org/LimeNET_Micro) (both through SoapySDR), BladeRF, other devices supported by [**gr-osmosdr**](https://osmocom.org/projects/sdr/wiki/GrOsmoSDR) like HackRF and RedPitaya (not tested)
 

Requirements
----

- Since release 0.8.5, GNU radio version 3.8 is necessary and Qt 5.14 is used for the graphical interface.
- Also since release 0.8.5 the video mode requires gstreamer and libgstreamer-plugins-bad1.0-0
- Build dependencies on Debian 11 with Qt5 and GNU radio 3.8: 

<pre>
$ sudo apt-get install gnuradio-dev protobuf-compiler gr-osmosdr gnuradio libvolk2-dev libvolk2-bin libprotobuf23 libprotobuf-dev libopus0 libopus-dev libspeexdsp1 libspeexdsp-dev libpulse0 libpulse-dev liblog4cpp5v5 libcodec2-0.9 libcodec2-dev libasound2 libasound2-dev libjpeg62-turbo libjpeg62-turbo-dev libconfig++9v5 libconfig++-dev qt5-qmake qtbase5-dev libqt5core5a libqt5gui5 libqt5network5 libqt5sql5 qtmultimedia5-dev libqt5multimediawidgets5 libqt5multimedia5-plugins libqt5multimedia5 libftdi-dev libftdi1 libsndfile1-dev libsndfile1 qtgstreamer-plugins-qt5 libgstreamer-plugins-bad1.0-0
</pre>

- Qt >= 5.14 and Qt5 development packages (older versions of Qt5 >= 5.11 might work as well)
- qmake
- Pulseaudio or Alsa or Jack
- Gnuradio >= 3.8 built with UHD, SoapySDR support and FreeDV/Codec2 support. Please verify that the following was printed before you (or your distribution's packager) built gnuradio:
<pre>
--   * gr-vocoder
--   * * codec2
--   * * freedv
</pre>

In [#67](https://github.com/qradiolink/qradiolink/issues/67) it was reported that it may be necessary to [set some cmake options](https://github.com/qradiolink/qradiolink/issues/67#issuecomment-706307297) In order for [codec2](https://github.com/drowe67/codec2) and it's freedv support to be detected properly.
- Boost and boost-devel
- libgnuradio-osmosdr (gr-osmosdr) built with UHD, RTL-SDR, SoapySDR, HackRF, RedPitaya or BladeRF support
- libprotobuf, libopus, libspeexdsp, libpulse-simple, libpulse, libasound, libcodec2, libsqlite3, libjpeg, libconfig++, libvolk, libftdi, libsndfile1, qtmultimedia5-dev, libqt5multimediawidgets5, libqt5multimedia5, libqt5multimedia5 (gstreamer1.0-plugins-bad for Qt video)
- protoc compiler (libprotoc 2.6.1 or greater, depending on which version of libprotobuf is used)
- SoapySDR and SoapyPlutoSDR, SoapyLMS7 are recommended as Soapy has support for many devices

In order to build on Ubuntu 17.10 you have to install the following packages, assuming a full GNU Radio development environment is already installed. Please note these instructions are for a very old version and are not guaranteed to work with newer versions. It is recommended to follow the Debian install guide and adjust for Ubuntu differences.

<pre>
$ sudo apt install libconfig++-dev libprotobuf-dev libopus-dev libpulse-dev libasound2-dev libcodec2-dev libsqlite3-dev libjpeg-dev libprotoc-dev libsndfile1 libftdi protobuf-compiler
</pre>

On Ubuntu 18.04 LTS, replace libjpeg62-turbo and libjpeg62-turbo-dev with libjpeg-turbo8-dev and libjpeg-dev
<pre>
$ sudo apt install libjpeg-turbo8-dev libjpeg-dev
</pre>

[Downloads](https://github.com/qradiolink/qradiolink/releases "Downloads")
----

Debian 11 Bullseye x86_64 packages are provided via automated builds. An AppImage for running the application on other Linux distributions with glibc >= 2.27 without installing it is also provided. The AppImage is based on Debian 11 Buster packages.
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
cd src/ext/
protoc --cpp_out=. Mumble.proto
protoc --cpp_out=. QRadioLink.proto
cd ../../build/
qmake ..
make
./qradiolink
</pre>

You can add custom library search paths, custom includes paths and specify a different install directory using these variables with qmake:
<pre>
qmake .. INSTALL_PREFIX=/usr/local/bin LIBDIR=/opt/lib INCDIR=/opt/include
</pre>



Operation
----

See docs/OPERATION.md

**Important note to Pulseaudio users: please use the Pulseaudio provided sinks and sources in the configuration dropdown, not the default device (or pulse)**

These audio sinks and sources should look like this:
- alsa_input.pci-0000_04_00.6.analog-stereo
- alsa_output.pci-0000_04_00.6.analog-stereo


Known issues
----

- When tuning rapidly for a prolonged time (for example by dragging the filter box across the screen), the audio may experience slowdowns and other glitches. As a workaround, click at the desired location on the waterfall instead of dragging.
- Due to an issue in libvolk / GNU radio 3.8, digital modes are broken unless the workaround documented in docs/OPERATION.md is applied.


Copyright and License
-------------------
- Most of the source code are released under the GNU General Public License version 3. Please see the COPYRIGHT and AUTHORS files for details.
- Parts of the code are licensed under the MIT license.
- Parts of the code are licensed under the BSD license.
- Some graphical resources are licensed under LGPLv3
- Some graphical resources are distributed under the Creative Commons Non-commercial, No-derivative 4.0 license.
- Sounds used by this project are copyright © 2020 w2sjw.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.



