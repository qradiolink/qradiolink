#-------------------------------------------------
#
# Project created by QtCreator 2015-08-17T21:06:30
#
#-------------------------------------------------

QT       += core gui network multimedia widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += multimediawidgets

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
} else {
    message(Building without Qt audio support)
    SOURCES += src/audio/audiointerface.cpp
    HEADERS += src/audio/audiointerface.h
    LIBS += -lpulse-simple -lpulse
}


SOURCES += main.cpp\
        src/mainwindow.cpp\
        src/audio/audioencoder.cpp\
        src/audio/audioprocessor.cpp \
        src/video/videoencoder.cpp \
        src/video/imagecapture.cpp \
        src/mumbleclient.cpp\
        src/layer2.cpp \
        src/audio/audiowriter.cpp \
        src/audio/audioreader.cpp \
        src/audio/audiorecorder.cpp \
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
        src/audio/audiomixer.cpp \
        src/telnetclient.cpp\
        src/ext/Mumble.pb.cc\
        src/ext/QRadioLink.pb.cc\
        src/ext/utils.cpp\
        src/ext/filt.cpp\
        src/ext/compressor.c \
        src/ext/snd.c \
        src/ext/mem.c \
        src/net/netdevice.cpp \
        src/layer1framing.cpp \
        src/limits.cpp \
    src/qtgui/freqctrl.cpp \
    src/qtgui/plotter.cpp \
    src/qtgui/skinneddial.cpp \
    src/gr/rssi_block.cpp \
    src/gr/gr_mod_base.cpp \
    src/gr/gr_demod_base.cpp \
    src/gr/gr_demod_nbfm_sdr.cpp \
    src/gr/gr_demod_freedv.cpp \
    src/gr/gr_mod_freedv.cpp \
    src/gr/gr_deframer_bb.cpp \
    src/gr/gr_audio_source.cpp \
    src/gr/gr_audio_sink.cpp \
    src/gr/gr_4fsk_discriminator.cpp \
    src/gr/gr_const_sink.cpp \
    src/gr/rx_fft.cpp \
    src/gr/emphasis.cpp \
    src/gr/dsss_encoder_bb_impl.cc \
    src/gr/dsss_decoder_cc_impl.cc \
    src/gr/gr_byte_source.cpp \
    src/gr/gr_bit_sink.cpp \
    src/gr/gr_sample_sink.cpp \
    src/gr/gr_demod_2fsk.cpp \
    src/gr/gr_demod_4fsk.cpp \
    src/gr/gr_demod_am.cpp \
    src/gr/gr_demod_bpsk.cpp \
    src/gr/gr_demod_dsss.cpp \
    src/gr/gr_demod_qpsk.cpp \
    src/gr/gr_demod_ssb.cpp \
    src/gr/gr_demod_wbfm.cpp \
    src/gr/gr_mod_2fsk.cpp \
    src/gr/gr_mod_4fsk.cpp \
    src/gr/gr_mod_am.cpp \
    src/gr/gr_mod_bpsk.cpp \
    src/gr/gr_mod_dsss.cpp \
    src/gr/gr_mod_nbfm.cpp \
    src/gr/gr_mod_qpsk.cpp \
    src/gr/gr_mod_ssb.cpp \
    src/gr/cessb/clipper_cc_impl.cc \
    src/gr/cessb/stretcher_cc_impl.cc



HEADERS  += src/mainwindow.h\
        src/audio/audioencoder.h\
        src/audio/audioprocessor.h \
        src/video/videoencoder.h \
        src/video/imagecapture.h \
        src/layer2.h \
        src/audio/audiowriter.h \
        src/audio/audioreader.h \
        src/audio/audiorecorder.h \
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
        src/audio/audiomixer.h \
        src/config_defines.h\
        src/ext/dec.h\
        src/ext/Mumble.pb.h\
        src/ext/PacketDataStream.h\
        src/ext/QRadioLink.pb.h\
        src/ext/utils.h \
        src/ext/filt.h \
        src/ext/snd.h \
        src/ext/mem.h \
        src/ext/compressor.h \
        src/net/netdevice.h \
        src/radiocontroller.h \
        src/layer1framing.h \
        src/limits.h \
    src/qtgui/freqctrl.h \
    src/qtgui/plotter.h \
    src/qtgui/skinneddial.h \
    src/gr/rssi_block.h \
    src/gr/gr_mod_base.h \
    src/gr/gr_demod_base.h \
    src/gr/gr_demod_freedv.h \
    src/gr/gr_mod_freedv.h \
    src/gr/rx_fft.h \
    src/gr/gr_deframer_bb.h \
    src/gr/gr_audio_source.h \
    src/gr/gr_audio_sink.h \
    src/gr/gr_4fsk_discriminator.h \
    src/gr/gr_const_sink.h \
    src/gr/emphasis.h \
    src/gr/dsss_encoder_bb_impl.h \
    src/gr/dsss_decoder_cc_impl.h \
    src/gr/gr_byte_source.h \
    src/gr/gr_bit_sink.h \
    src/gr/gr_sample_sink.h \
    src/gr/gr_demod_2fsk.h \
    src/gr/gr_demod_4fsk.h \
    src/gr/gr_demod_am.h \
    src/gr/gr_demod_bpsk.h \
    src/gr/gr_demod_dsss.h \
    src/gr/gr_demod_nbfm.h \
    src/gr/gr_demod_qpsk.h \
    src/gr/gr_demod_ssb.h \
    src/gr/gr_demod_wbfm.h \
    src/gr/gr_mod_2fsk.h \
    src/gr/gr_mod_4fsk.h \
    src/gr/gr_mod_am.h \
    src/gr/gr_mod_bpsk.h \
    src/gr/gr_mod_dsss.h \
    src/gr/gr_mod_nbfm.h \
    src/gr/gr_mod_qpsk.h \
    src/gr/gr_mod_ssb.h \
    src/gr/cessb/api.h \
    src/gr/cessb/clipper_cc.h \
    src/gr/cessb/stretcher_cc.h \
    src/gr/cessb/clipper_cc_impl.h \
    src/gr/cessb/stretcher_cc_impl.h \
    src/style.h



!isEmpty(LIBDIR) {
    LIBS += -L$$LIBDIR
}
!isEmpty(INCDIR) {
    INCLUDEPATH += $$INCDIR
}


#CONFIG += link_pkgconfig
#PKGCONFIG += gnuradio

FORMS    += src/mainwindow.ui


LIBS += -lgnuradio-pmt -lgnuradio-analog -lgnuradio-fft -lgnuradio-vocoder \
        -lgnuradio-osmosdr -lvolk \
        -lgnuradio-blocks -lgnuradio-filter -lgnuradio-digital -lgnuradio-runtime -lgnuradio-fec \
        -lboost_system$$BOOST_SUFFIX
LIBS += -lrt  # need to include on some distros
LIBS += -lprotobuf -lopus -lcodec2 -ljpeg -lconfig++ -lspeexdsp -lftdi -lsndfile -llog4cpp


RESOURCES += src/resources.qrc

!isEmpty(INSTALL_PREFIX) {
    target.path = $$INSTALL_PREFIX
    INSTALLS += target
}

