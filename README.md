QRadioLink
==========

[![Build Status](https://travis-ci.org/qradiolink/qradiolink.svg)](https://travis-ci.org/qradiolink/qradiolink)

[![Packaging status](https://repology.org/badge/vertical-allrepos/qradiolink.svg)](https://repology.org/project/qradiolink/versions)


Supported hardware
---

-  [**Ettus USRP bus devices**](https://ettus.com), [**RTL-SDR**](https://osmocom.org/projects/sdr/wiki/rtl-sdr), [**ADALM-Pluto (PlutoSDR)**](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/adalm-pluto.html), (supported with SoapySDR and [**SoapyPlutoSDR**](https://github.com/pothosware/SoapyPlutoSDR)), [**LimeSDR-mini**](https://www.crowdsupply.com/lime-micro/limesdr-mini), [**LimeNET-Micro**](https://wiki.myriadrf.org/LimeNET_Micro) (both through SoapySDR), BladeRF, other devices supported by [**gr-osmosdr**](https://osmocom.org/projects/sdr/wiki/GrOsmoSDR) like HackRF and RedPitaya (not tested)
 

Requirements
----

- Since release 0.8.5, GNU radio version is changed to 3.8 and Qt 5.14 is used for the graphical interface. Debian Bullseye is the base GNU/Linux distribution for which packages are built.
- Build dependencies on Debian Bullseye with Qt5 and GNU radio 3.8: 

<pre>
$ sudo apt-get install gnuradio-dev protobuf-compiler gr-osmosdr gnuradio libvolk2-dev libvolk2-bin libprotobuf23 libprotobuf-dev libopus0 libopus-dev libspeexdsp1 libspeexdsp-dev libpulse0 libpulse-dev liblog4cpp5v5 libcodec2-0.9 libcodec2-dev libasound2 libasound2-dev libjpeg62-turbo libjpeg62-turbo-dev libconfig++9v5 libconfig++-dev qt5-qmake qt5-default qtbase5-dev libqt5core5a libqt5gui5 libqt5network5 libqt5sql5 qtmultimedia5-dev libqt5multimediawidgets5 libqt5multimedia5-plugins libqt5multimedia5 libftdi-dev libftdi1 libsndfile1-dev libsndfile1
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
- optionally SoapySDR and SoapyPlutoSDR, SoapyLMS7

In order to build on Ubuntu 17.10 you have to install the following packages, assuming a full GNU Radio development environment is already installed. Please note these instructions are for a very old version of QRadioLink and are not guaranteed to work with newer versions. It is recommended to follow the Debian install guide and adjust for Ubuntu differences.

<pre>
$ sudo apt install libconfig++-dev libprotobuf-dev libopus-dev libpulse-dev libasound2-dev libcodec2-dev libsqlite3-dev libjpeg-dev libprotoc-dev libsndfile1 libftdi protobuf-compiler
</pre>

On Ubuntu 18.04 LTS, replace libjpeg62-turbo and libjpeg62-turbo-dev with libjpeg-turbo8-dev and libjpeg-dev
<pre>
$ sudo apt install libjpeg-turbo8-dev libjpeg-dev
</pre>

[Downloads](https://github.com/qradiolink/qradiolink/releases "Downloads")
----

Debian 11 Bullseye x86_64 packages are provided via Travis CI automated builds. An AppImage for running the application on other Linux distributions with glibc >= 2.27 without installing it is also provided for older releases based on GNU radio 3.7. The AppImage is based on Debian 10 Buster packages.
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

Known issues:
- IP modems operating in burst mode experience some packet loss due to lost frames.
- FM CTCSS decoder is not very reliable

Operation
----

See docs/README.md


Credits and License
-------------------
- QRadioLink is released under an Open Source License, the GNU General Public License version 3.
Authors and contributors are listed in the AUTHORS and CREDITS file.


