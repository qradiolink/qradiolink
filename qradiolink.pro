#-------------------------------------------------
#
# Project created by QtCreator 2015-08-17T21:06:30
#
#-------------------------------------------------

QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qradiolink
TEMPLATE = app

CONFIG  += qt thread qtaudio

#QMAKE_CXXFLAGS += -Werror
QMAKE_CXXFLAGS += $$(CXXFLAGS)
QMAKE_CFLAGS += $$(CFLAGS)
QMAKE_LFLAGS += $$(LDFLAGS)

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
    SOURCES += video/imagecapture.cpp
    HEADERS += video/imagecapture.h
} else {
    message(Building without Qt audio support)
    SOURCES += audio/audiointerface.cpp
    HEADERS += audio/audiointerface.h
    LIBS += -lpulse-simple -lpulse
}


SOURCES += main.cpp\
        mainwindow.cpp\
        audio/audioencoder.cpp\
        audio/audioprocessor.cpp \
        video/videoencoder.cpp \
        video/videocapture.cpp \
        src/mumbleclient.cpp\
        src/layer2.cpp \
        audio/audiowriter.cpp \
        audio/audioreader.cpp \
        audio/audiorecorder.cpp \
        src/mumblechannel.cpp \
        src/radiochannel.cpp \
        src/relaycontroller.cpp \
        src/radiocontroller.cpp \
        src/commandprocessor.cpp \
        src/telnetserver.cpp \
        src/settings.cpp\
        src/sslclient.cpp\
        src/station.cpp\
        src/logger.cpp \
        src/gr_modem.cpp \
        audio/audiomixer.cpp \
        src/telnetclient.cpp\
        ext/Mumble.pb.cc\
        ext/QRadioLink.pb.cc\
        ext/utils.cpp\
        ext/filt.cpp\
        ext/compressor.c \
        ext/snd.c \
        ext/mem.c \
        net/netdevice.cpp \
        src/layer1framing.cpp \
        src/limits.cpp \
    qtgui/freqctrl.cpp \
    qtgui/plotter.cpp \
    qtgui/skinneddial.cpp \
    gr/gr_mod_base.cpp \
    gr/gr_demod_base.cpp \
    gr/gr_demod_nbfm_sdr.cpp \
    gr/gr_demod_freedv.cpp \
    gr/gr_mod_freedv.cpp \
    gr/gr_deframer_bb.cpp \
    gr/gr_audio_source.cpp \
    gr/gr_audio_sink.cpp \
    gr/gr_4fsk_discriminator.cpp \
    gr/gr_const_sink.cpp \
    gr/rx_fft.cpp \
    gr/emphasis.cpp \
    gr/dsss_encoder_bb_impl.cc \
    gr/dsss_decoder_cc_impl.cc \
    gr/gr_byte_source.cpp \
    gr/gr_bit_sink.cpp \
    gr/gr_sample_sink.cpp \
    gr/gr_demod_2fsk.cpp \
    gr/gr_demod_4fsk.cpp \
    gr/gr_demod_am.cpp \
    gr/gr_demod_bpsk.cpp \
    gr/gr_demod_dsss.cpp \
    gr/gr_demod_qpsk.cpp \
    gr/gr_demod_ssb.cpp \
    gr/gr_demod_wbfm.cpp \
    gr/gr_mod_2fsk.cpp \
    gr/gr_mod_4fsk.cpp \
    gr/gr_mod_am.cpp \
    gr/gr_mod_bpsk.cpp \
    gr/gr_mod_dsss.cpp \
    gr/gr_mod_nbfm.cpp \
    gr/gr_mod_qpsk.cpp \
    gr/gr_mod_ssb.cpp



HEADERS  += mainwindow.h\
        audio/audioencoder.h\
        audio/audioprocessor.h \
        video/videoencoder.h \
        src/layer2.h \
        audio/audiowriter.h \
        audio/audioreader.h \
        audio/audiorecorder.h \
        src/mumblechannel.h \
        src/radiochannel.h \
        src/relaycontroller.h \
        src/commandprocessor.h \
        src/mumbleclient.h\
        src/telnetserver.h \
        src/settings.h\
        src/sslclient.h\
        src/station.h\
        src/telnetclient.h\
        src/logger.h \
        src/modem_types.h \
        src/gr_modem.h \
        audio/audiomixer.h \
        src/config_defines.h\
        ext/dec.h\
        ext/Mumble.pb.h\
        ext/PacketDataStream.h\
        ext/QRadioLink.pb.h\
        ext/utils.h \
        ext/filt.h \
        ext/snd.h \
        ext/mem.h \
        ext/compressor.h \
        net/netdevice.h \
        src/radiocontroller.h \
        src/layer1framing.h \
        src/limits.h \
    qtgui/freqctrl.h \
    qtgui/plotter.h \
    qtgui/skinneddial.h \
    gr/gr_mod_base.h \
    gr/gr_demod_base.h \
    gr/gr_demod_freedv.h \
    gr/gr_mod_freedv.h \
    gr/rx_fft.h \
    gr/gr_deframer_bb.h \
    gr/gr_audio_source.h \
    gr/gr_audio_sink.h \
    gr/gr_4fsk_discriminator.h \
    gr/gr_const_sink.h \
    gr/emphasis.h \
    gr/dsss_encoder_bb_impl.h \
    gr/dsss_decoder_cc_impl.h \
    gr/gr_byte_source.h \
    gr/gr_bit_sink.h \
    gr/gr_sample_sink.h \
    gr/gr_demod_2fsk.h \
    gr/gr_demod_4fsk.h \
    gr/gr_demod_am.h \
    gr/gr_demod_bpsk.h \
    gr/gr_demod_dsss.h \
    gr/gr_demod_nbfm.h \
    gr/gr_demod_qpsk.h \
    gr/gr_demod_ssb.h \
    gr/gr_demod_wbfm.h \
    gr/gr_mod_2fsk.h \
    gr/gr_mod_4fsk.h \
    gr/gr_mod_am.h \
    gr/gr_mod_bpsk.h \
    gr/gr_mod_dsss.h \
    gr/gr_mod_nbfm.h \
    gr/gr_mod_qpsk.h \
    gr/gr_mod_ssb.h \
    style.h



!isEmpty(LIBDIR) {
    LIBS += -L$$LIBDIR
}
!isEmpty(INCDIR) {
    INCLUDEPATH += $$INCDIR
}


#CONFIG += link_pkgconfig
#PKGCONFIG += gnuradio

FORMS    += mainwindow.ui


LIBS += -lgnuradio-pmt -lgnuradio-analog -lgnuradio-fft -lgnuradio-vocoder \
        -lgnuradio-osmosdr -lvolk \
        -lgnuradio-blocks -lgnuradio-filter -lgnuradio-digital -lgnuradio-runtime -lgnuradio-fec \
        -lboost_system$$BOOST_SUFFIX
LIBS += -lrt  # need to include on some distros
LIBS += -lprotobuf -lopus -lcodec2 -ljpeg -lconfig++ -lspeexdsp -lftdi -lsndfile -llog4cpp


RESOURCES += resources.qrc

!isEmpty(INSTALL_PREFIX) {
    target.path = $$INSTALL_PREFIX
    INSTALLS += target
}

