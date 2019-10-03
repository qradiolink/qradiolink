#-------------------------------------------------
#
# Project created by QtCreator 2015-08-17T21:06:30
#
#-------------------------------------------------

QT       += core gui network sql multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qradiolink
TEMPLATE = app

CONFIG  += qt thread

message($$QMAKESPEC)

linux-g++ {
    message(Building for GNU/Linux)
}

CONFIG(opengl) {
    message(Building with OpenGL support.)
    DEFINES += USE_OPENGL_PLOTTER
} else {
    message(Building without OpenGL support)
}

CONFIG(qtaudio) {
    message(Building with Qt audio support.)
    DEFINES += USE_QT_AUDIO
} else {
    message(Building without Qt audio support)
}

#QMAKE_CXXFLAGS += -Werror
QMAKE_CXXFLAGS += $$(CXXFLAGS)
QMAKE_CFLAGS += $$(CFLAGS)
QMAKE_LFLAGS += $$(LDFLAGS)

SOURCES += main.cpp\
        mainwindow.cpp\
        audio/audioencoder.cpp\
        audio/audiointerface.cpp\
        audio/audioprocessor.cpp \
        dtmfdecoder.cpp\
        mumbleclient.cpp\
        radioprotocol.cpp \
        audiowriter.cpp \
        audioreader.cpp \
        mumblechannel.cpp \
        radiochannel.cpp \
        relaycontroller.cpp \
        radiocontroller.cpp \
        server.cpp\
        settings.cpp\
        sslclient.cpp\
        station.cpp\
        telnetclient.cpp\
        ext/Mumble.pb.cc\
        ext/QRadioLink.pb.cc\
        ext/utils.cpp\
        ext/filt.cpp\
        ext/compressor.c \
        ext/snd.c \
        ext/mem.c \
        audio/alsaaudio.cpp \
        video/videocapture.cpp \
        video/videoencoder.cpp \
        net/netdevice.cpp \
    qtgui/freqctrl.cpp \
    qtgui/plotter.cpp \
    gr/gr_modem.cpp \
    gr/gr_vector_source.cpp \
    gr/gr_vector_sink.cpp \
    gr/gr_demod_bpsk_sdr.cpp \
    gr/gr_mod_bpsk_sdr.cpp \
    gr/gr_mod_qpsk_sdr.cpp \
    gr/gr_demod_qpsk_sdr.cpp \
    gr/gr_mod_4fsk_sdr.cpp \
    gr/gr_demod_4fsk_sdr.cpp \
    gr/gr_mod_base.cpp \
    gr/gr_demod_base.cpp \
    gr/gr_mod_nbfm_sdr.cpp \
    gr/gr_demod_nbfm_sdr.cpp \
    gr/gr_demod_wbfm_sdr.cpp \
    gr/gr_mod_ssb_sdr.cpp \
    gr/gr_demod_ssb_sdr.cpp \
    gr/gr_mod_am_sdr.cpp \
    gr/gr_demod_am_sdr.cpp \
    gr/gr_mod_2fsk_sdr.cpp \
    gr/gr_demod_2fsk_sdr.cpp \
    gr/gr_demod_freedv.cpp \
    gr/gr_mod_freedv.cpp \
    gr/gr_deframer_bb.cpp \
    gr/gr_audio_source.cpp \
    gr/gr_audio_sink.cpp \
    gr/gr_4fsk_discriminator.cpp \
    gr/gr_const_sink.cpp \
    gr/rx_fft.cpp



HEADERS  += mainwindow.h\
        audio/audioencoder.h\
        audio/audiointerface.h\
        audio/audioprocessor.h \
        radioprotocol.h \
        audiowriter.h \
        audioreader.h \
        mumblechannel.h \
        radiochannel.h \
        relaycontroller.h \
        dtmfdecoder.h\
        mumbleclient.h\
        server.h\
        settings.h\
        sslclient.h\
        station.h\
        telnetclient.h\
        config_defines.h\
        ext/dec.h\
        ext/Goertzel.h\
        ext/Mumble.pb.h\
        ext/PacketDataStream.h\
        ext/QRadioLink.pb.h\
        ext/utils.h \
        ext/filt.h \
        ext/snd.h \
        ext/mem.h \
        ext/compressor.h \
        video/videoencoder.h \
        net/netdevice.h \
        radiocontroller.h \
        audio/alsaaudio.h \
    qtgui/freqctrl.h \
    qtgui/plotter.h \
    gr/gr_modem.h \
    gr/gr_vector_source.h \
    gr/gr_vector_sink.h \
    gr/gr_demod_bpsk_sdr.h \
    gr/gr_mod_bpsk_sdr.h \
    gr/gr_mod_qpsk_sdr.h \
    gr/gr_demod_qpsk_sdr.h \
    gr/gr_mod_4fsk_sdr.h \
    gr/gr_demod_4fsk_sdr.h \
    gr/gr_mod_base.h \
    gr/gr_demod_base.h \
    gr/gr_mod_nbfm_sdr.h \
    gr/gr_demod_nbfm_sdr.h \
    gr/gr_demod_wbfm_sdr.h \
    gr/gr_mod_ssb_sdr.h \
    gr/gr_demod_ssb_sdr.h \
    gr/gr_mod_am_sdr.h \
    gr/gr_demod_am_sdr.h \
    gr/gr_mod_2fsk_sdr.h \
    gr/gr_demod_2fsk_sdr.h \
    gr/gr_demod_freedv.h \
    gr/gr_mod_freedv.h \
    gr/rx_fft.h \
    gr/gr_deframer_bb.h \
    gr/gr_audio_source.h \
    gr/gr_audio_sink.h \
    gr/gr_4fsk_discriminator.h \
    gr/gr_const_sink.h \
    gr/modem_types.h



#CONFIG += link_pkgconfig
#PKGCONFIG += gnuradio

FORMS    += mainwindow.ui


LIBS += -lgnuradio-pmt -lgnuradio-audio -lgnuradio-analog -lgnuradio-blocks -lgnuradio-fft -lgnuradio-vocoder \
        -lgnuradio-osmosdr -lvolk \
        -lgnuradio-blocks -lgnuradio-filter -lgnuradio-digital -lgnuradio-runtime -lgnuradio-fec \
        -lboost_thread$$BOOST_SUFFIX -lboost_system$$BOOST_SUFFIX -lboost_program_options$$BOOST_SUFFIX
LIBS += -lrt  # need to include on some distros

LIBS += -lprotobuf -lopus -lpulse-simple -lpulse -lcodec2 -lasound -ljpeg -lconfig++ -lspeexdsp -lftdi
                    #-lFestival -lestbase -leststring -lestools
#INCLUDEPATH += /usr/include/speech_tools


RESOURCES += resources.qrc
