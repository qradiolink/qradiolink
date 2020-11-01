About
-----

*QRadioLink* is a VOIP (radio over IP) GNU/Linux SDR (software defined radio) transceiver application using Internet protocols for communication,
built on top of [GNU radio](https://www.gnuradio.org/), 
which allows experimenting with software defined radio hardware using different digital and analog radio signals and a Qt5 user interface.

Its primary purpose is educational, but it can also be customized for low power data communications on various ISM frequency bands.
It can also be used as a low power amateur radio SDR transceiver for demonstrating radio communications to children at schools.

The application was originally inspired from the [Codec2 GMSK modem](https://github.com/on1arf/gmsk) project by Kristoff Bonne.

[![Screenshot](http://qradiolink.org/images/qradiolink72.png)](http://qradiolink.org)

Features
---

- VOIP (Radio-over-IP) connection between two or more stations operating in simplex or semi-duplex mode
- Radio forwarding over VOIP - forward voice to the VOIP connection and viceversa
- Direct VOIP talk-around (only requires connection to a VOIP server and no radio)
- Wideband audio streaming over the Internet with the **Opus** audio codec
- Configurable VOIP bitrate
- Audio recording for local audio output in FLAC format
- Remote control via network (requires a telnet client or similar program, can be scripted)
- Remote control via Mumble private text messages
- Run headless (no graphical user interface) for terminal usage on embedded platforms like the Raspberry Pi or similar boards without any screen
- Night mode theme
- Transmit and receive analog FM, SSB, AM, digital voice, text messages, digital video, IP protocol.
- Mixed operation mode (receive one mode and transmit another)
- Full duplex and simplex operation
- Fast tune reverse repeater shift using dedicated button
- Memory channels (store frequency, name, TX shift, operating mode, squelch value, volume, TX power, RX gain, TX and RX CTCSS) and memory channel scan 
- Split operation (transmit on other frequency than the receive frequency with no shift limitation, used mostly for repeater operation)
- Digital voice codecs: Codec2 700 bit/s, Codec2 1400 bit/s, Opus 9600 bit/s
- FreeDV digital voice modulator and demodulator (currently supports only 1600, 700C `and 800XA modes)
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
- Full duplex DQPSK 250 kbit/s and 4FSK 96 kbit/s IP radio modems with configurable TX/RX offsets
- User paging
- Automatic carrier tracking and Doppler effect correction for all digital modes except FreeDV modes. The system can track Doppler shifts of 5-10 kHz, depending on mode. It requires a CNR of at least 10-12 dB, more for FSK modes than for PSK modes. 
