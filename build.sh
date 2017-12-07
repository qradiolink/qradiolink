#!/bin/bash

mkdir build

pushd ext
protoc --cpp_out=. Mumble.proto
protoc --cpp_out=. QRadioLink.proto
popd

pushd build
export PKGCONFS="""
       gnuradio-runtime
       gnuradio-audio gnuradio-analog gnuradio-blocks gnuradio-digital gnuradio-filter gnuradio-qtgui gnuradio-fec
       gnuradio-osmosdr
       protobuf libconfig++
       opus codec2
       libpulse libpulse-simple alsa
       libjpeg
       qwt5-qt4
"""
export CXXFLAGS=$(pkg-config --cflags ${PKGCONFS})
export LIBFLAGS=$(pkg-config --libs $PKGCONFS)
echo "CXXFLAGS: ${CXXFLAGS}"
echo "LDFLAGS: ${LDFLAGS}"
qmake-qt4 ..
#make -j$(( $(nproc) + 1 ))
make -j1
