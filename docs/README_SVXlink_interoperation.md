 
# SVXlink and UDP audio streaming operation mode

QRadioLink can now interoperate with SVXlink and other applications which use UDP audio streaming.
Voice forwarding to and from Mumble and UDP audio streaming are not mutually exclusive. If both options are enabled, demodulated audio will be sent to both UDP and Mumble, while voice originating from Mumble will be mixed with voice originating from UDP and the result will be transmitted over RF.
There are several new settings for the configuration of UDP streaming and SVXlink control:

* **UDP audio sample rate** (default 16000): possible values are 48000, 16000, 8000. Audio is resampled from the internal rate of 8000 samples per second to the rate defined here. To communicate with SVXlink, the rate set here needs to match the setting CARD_SAMPLE_RATE in the [GLOBAL] section of the svxlink config. Also important, the other application needs to stream only mono audio (only one channel, not interleaved samples from two channels). In svxlink this is achieved by setting CARD_CHANNELS=1 in the [GLOBAL] section.

* **UDP audio RX port** (default 4937). This is the UDP port where demodulated audio samples are sent to. Needs to match the port defined in the streaming application. In svxlink, the [Rx1] audio device option will look like this: AUDIO_DEV=udp:127.0.0.1:4937

* **UDP audio TX port** (default 4938). This is the UDP port where qradiolink listens for audio samples to be transmitted over the air. Needs to match the port defined for TX in the streaming application. In svxlink, the [Tx1] audio device option will look like this: AUDIO_DEV=udp:127.0.0.1:4938

* **SVXlink squelch PTY path** (default /tmp/sql_pty): in the General settings tab, this is the path of a pseudo-tty used by SVXlink to control squelch. Needs to match setting PTY_PATH in the [Rx1] section of svxlink. Also, the squelch detector in svxlink needs to be set to PTY: SQL_DET=PTY (also in the [Rx1] section).

* **UDP audio**: this button (in the Base station tab) needs to be enabled once RX and TX have been enabled in qradiolink.
If starting qradiolink headless from the command line, use this command argument to enable UDP streaming:
<pre>qradiolink --headless --udp</pre>

* **Duplex**: this button should be disabled when using svxlink as a simplex node. Also the TX shift (Split kHz) needs to be 0.

* **VOIP volume**: Outgoing (towards SVXlink) UDP audio volume is controlled by this slider. Should be set around 50% most of the time

* **Microphone gain**: Incoming UDP audio volume (from SVXlink towards RF) is controlled by this slider. Most of the time should be at around 50%

# Relevant QRadioLink config values to be adjusted

<pre>
squelch = -75;
rx_volume = 80;
tx_volume = 50;
voip_volume = 58;
tx_shift = 0L;
enable_duplex = 0;
udp_listen_port = 4938;
udp_send_port = 4937;
tot_tx_end = 0;
udp_audio_sample_rate = 16000;
sql_pty_path = "/tmp/sql_pty";
</pre>

SVXlink in repeater mode has not yet been tested. For it to work, one needs to set duplex on in qradiolink and set a TX shift equal to the repeater split.

# Example working svxlink client config for a simplex node

<pre>
[GLOBAL]
MODULE_PATH=/usr/lib/x86_64-linux-gnu/svxlink
LOGICS=SimplexLogic,ReflectorLogic
CFG_DIR=/etc/svxlink/svxlink.d
TIMESTAMP_FORMAT="%c"
CARD_SAMPLE_RATE=16000
CARD_CHANNELS=1
#LOCATION_INFO=LocationInfo
LINKS=ReflectorUplink

[SimplexLogic]
TYPE=Simplex
RX=Rx1
TX=Tx1
MODULES=
CALLSIGN=XX0XXX
SHORT_IDENT_INTERVAL=10
LONG_IDENT_INTERVAL=30
#IDENT_ONLY_AFTER_TX=4
#EXEC_CMD_ON_SQL_CLOSE=500
EVENT_HANDLER=/usr/share/svxlink/events.tcl
DEFAULT_LANG=en_US
RGR_SOUND_DELAY=10
RGR_SOUND_ALWAYS=0
REPORT_CTCSS=103.5
#TX_CTCSS=ALWAYS
MACROS=Macros
FX_GAIN_NORMAL=0
FX_GAIN_LOW=-12
#ACTIVATE_MODULE_ON_LONG_CMD=4:EchoLink
#QSO_RECORDER=8:QsoRecorder
#ONLINE_CMD=998877
MUTE_RX_ON_TX=1
MUTE_TX_ON_RX=1

[ReflectorLogic]
TYPE=Reflector
HOST=localhost
PORT=5302
CALLSIGN="xx0xxx"
AUTH_KEY="password"
JITTER_BUFFER_DELAY=500
TG_SELECT_TIMEOUT=99999

[ReflectorUplink]
CONNECT_LOGICS=SimplexLogic:99:XX0XXX,ReflectorLogic
DEFAULT_ACTIVE=1
TIMEOUT=10

[Rx1]
TYPE=Local
#RX_ID=?
AUDIO_DEV=udp:127.0.0.1:4937
AUDIO_CHANNEL=0
AUDIO_DEV_KEEP_OPEN=0
SQL_DET=PTY
SQL_START_DELAY=100
SQL_DELAY=40
SQL_HANGTIME=0
#SQL_EXTENDED_HANGTIME=1000
#SQL_EXTENDED_HANGTIME_THRESH=15
SQL_TIMEOUT=1000
VOX_FILTER_DEPTH=10
VOX_THRESH=1
#CTCSS_MODE=2
CTCSS_FQ=103.5
#CTCSS_SNR_OFFSET=0
#CTCSS_OPEN_THRESH=15
#CTCSS_CLOSE_THRESH=9
#CTCSS_BPF_LOW=60
#CTCSS_BPF_HIGH=270
#SERIAL_PORT=/dev/ttyS0
SERIAL_PIN=CTS
#SERIAL_SET_PINS=DTR!RTS
#EVDEV_DEVNAME=/dev/input/by-id/usb-SYNIC_SYNIC_Wireless_Audio-event-if03
#EVDEV_OPEN=1,163,1
#EVDEV_CLOSE=1,163,0
#GPIO_PATH=/sys/class/gpio
#GPIO_SQL_PIN=gpio30
PTY_PATH=/tmp/sql_pty
#HID_DEVICE=/dev/hidraw3
#HID_SQL_PIN=VOL_UP
SIGLEV_DET=NOISE
SIGLEV_SLOPE=1
SIGLEV_OFFSET=0
#SIGLEV_BOGUS_THRESH=120
#TONE_SIGLEV_MAP=100,84,60,50,37,32,28,23,19,8
SIGLEV_OPEN_THRESH=30
SIGLEV_CLOSE_THRESH=10
DEEMPHASIS=0
#SQL_TAIL_ELIM=300
#PREAMP=6
PEAK_METER=1
DTMF_DEC_TYPE=INTERNAL
DTMF_MUTING=1
DTMF_HANGTIME=40
DTMF_SERIAL=/dev/ttyS0
#DTMF_PTY=/tmp/rx1_dtmf
#DTMF_MAX_FWD_TWIST=8
#DTMF_MAX_REV_TWIST=4
#1750_MUTING=1
#SEL5_DEC_TYPE=INTERNAL
#SEL5_TYPE=ZVEI1
#FQ=433475000
#MODULATION=FM
#WBRX=WbRx1
#OB_AFSK_ENABLE=0
#OB_AFSK_VOICE_GAIN=6
#IB_AFSK_ENABLE=0

[Tx1]
TYPE=Local
AUDIO_DEV=udp:127.0.0.1:4938
AUDIO_CHANNEL=0
AUDIO_DEV_KEEP_OPEN=0
PTT_TYPE=NONE
PTT_PORT=/dev/ttyS0
PTT_PIN=DTRRTS
#HID_DEVICE=/dev/hidraw3
#HID_PTT_PIN=GPIO3
#SERIAL_SET_PINS=DTR!RTS
#GPIO_PATH=/sys/class/gpio
#PTT_HANGTIME=1000
TIMEOUT=1000
TX_DELAY=0
#CTCSS_FQ=136.5
#CTCSS_LEVEL=9
PREEMPHASIS=0
DTMF_TONE_LENGTH=100
DTMF_TONE_SPACING=50
DTMF_DIGIT_PWR=-15
#MASTER_GAIN=0.0
#OB_AFSK_ENABLE=0
#OB_AFSK_VOICE_GAIN=-6
#OB_AFSK_LEVEL=-12
#OB_AFSK_TX_DELAY=100
#IB_AFSK_ENABLE=0
#IB_AFSK_LEVEL=-6
#IB_AFSK_TX_DELAY=100
</pre>
