
# MMDVM operation mode

QRadioLink can act as a SDR base station transceiver for DMR, System Fusion, D-Star or M17
by connecting to MMDVM-SDR, a fork of MMDVM which runs on Linux operating systems. 
MMDVM-SDR is slightly different from the original MMDVM software and is tested and developed at https://codeberg.org/qradiolink/MMDVM-SDR on the `mmdvm_sdr` branch. A fork of MMDVMHost is also necessary, which is maintained at https://codeberg.org/qradiolink/MMDVMHost-SDR on the `mmdmv_sdr` branch.

# DMR tier III trunked radio base station mode

QRadioLink can also act as a DMR tier III trunked radio base station using MMDVM-SDR, MMDVMHost-SDR, DMRGateway and dmrtc (https://codeberg.org/qradiolink/dmrtc). This mode requires using the **trunking** branch from MMDVM-SDR, MMDVMHost-SDR and DMRGateway, and the master branch of dmrtc.

# Single and multi-carrier

QRadioLink can work either as a normal single channel transceiver or as a
multi-channel / multi-carrier transceiver. So far only DMR, System Fusion and M17 have been tested
and confirmed to work, but D-Star and other digital modes supported by MMDVM should work as well. 

Multi-carrier and single carrier MMDVM modes are only supported with LimeSDR and Ettus USRP devices so far due to the need for device timestamps used in TDMA mode. The LimeNet-Micro, the LimeSDR-mini and the USRP B20X were tested and confirmed to function in this mode. Other UHD based devices (AntSDR?) may also work but were not tested.

Support for other devices like BladeRF may be added in the future. If you have such hardware and are interested in getting it working, please create a pull request based on the `next` branch.

Due to performance issues, MMDVM multi-carrier modes do not work very well on ARM Linux platforms like Raspberry Pi. Some work is currently done to attempt to address this, but at the moment it is recommended to run MMDVM modes on x86_64 platforms with at least 4 cores.

## Installation


### 1. QRadioLink.
----

Install normally (see README.md)

----

### 2. MMDVM
----

- **Important:** you need to use the CMake build system, not the original Makefile, so use cmake instead of make

<pre>
git clone https://codeberg.org/qradiolink/MMDVM-SDR
cd MMDVM-SDR
git checkout mmdvm_sdr # for DMR tier III use the trunking branch
mkdir build && cd build
cmake ..
make -j 2
</pre>
----

### 3. MMDVMHost
----

<pre>
git clone https://codeberg.org/qradiolink/MMDVMHost-SDR
cd MMDVMHost-SDR
git checkout mmdvm_sdr # for DMR tier III use the trunking branch
make -j 4
</pre>
----

### 4. DMRTC (trunking controller)
----

<pre>
git clone https://codeberg.org/qradiolink/dmrtc
cd dmrtc
git checkout master
make -j 4
</pre>
----

### 5. DMRGateway (only used for DMR tier III trunking)
----

<pre>
git clone https://codeberg.org/qradiolink/DMRGateway
cd DMRGateway
git checkout trunking
make -j 4
</pre>
----

## Configuring and running the transceiver

The startup order is 

1. MMDVM via CLI or start.sh script
2. QRadioLink via CLI with --mmdvm as a CLI parameter
3. MMDVMHost instances for all channels

### 1. MMDVM
----

* For single channel mode: from the build/ directory, start mmdvm in single channel mode using:
<pre>
./mmdvm
</pre>
This will create the symlink to the virtual pts, which you will need to put into MMDVM.ini, section Modem:
<pre>
M: 2022-08-13 11:37:34.779 virtual pts: /dev/pts/15 <> /home/user1/MMDVM-SDR/build/ttyMMDVM0
</pre>

* If you have started in single channel mode, the tty will be called **ttyMMDVM0**

* For multi-channel mode: start multiple instances of mmdvm using the **-c CHANNEL_NUMBER** flag (requires multichannel mode also configured in QRadioLink). Start multiple mmdvm in different terminals like this (up to 7 channels can be used, numbered here from 1 to 7):
<pre>
./mmdvm -c 1
./mmdvm -c 2
./mmdvm -c 3
...
</pre>

* Verify in mmdvm output that the ttyMMDVM has the correct number (1 to 3 in above example)

* Channel 1 for mmdvm corresponds to qradiolink channel zero (main frequency) and so forth.
----

### 2. QRadioLink
----

* Start the application normally. Configure the LimeSDR or USRP device, as well as all other options.
Use a device string like **driver=lime,serial=00583A34CAD205** for LimeSDR or **driver=uhd,serial=00583A3** for USRP devices (replace serial with your own). Note serial is only needed if you have multiple devices.
You can find out the serial number by running LimeUtil, which is part of the LimeSuite package or uhd_find_devices:
<pre>
LimeUtil --find
</pre>

* In the Setup -> Radio settings page, choose the number of MMDVM channels to be transmitted, if operating in MMDVM multi-channel mode. This number needs to be between 2 and 7 at the moment (default is 3). The number of channels which can be transmitted simultaneously is heavily dependent on the CPU power. On small ARM platforms, due to the high sample rates involved internally, it may not be possible to operate more than 1 or 2 channels.
**Important**: you will need to start as many instances of MMDVM as many channels are configured here, otherwise qradiolink will not transmit anything.

* Select channel separation (options are currently 12.5 kHz and 25 kHz). Please note that only the 25 kHz channel separation for multicarrier mode has been tested so far and is recommended.

* Configure the RX frequency which will be the uplink from the radio. In the config file, **rx_frequency** and **demod_offset**, added together, will provide your baseband receive frequency. In multicarrier mode, demod_offset should be small (less than 50 kHz) and chosen such that DC does not fall in a used channel. With a channel separation of 25 kHz, demod_offset can be 12500 or -12500, but check that adding it to rx_frequency results in your wanted receive frequency for channel zero.

* Set the TX offset (Split) such that the TX frequency of the SDR will be the RX frequency
of the radio. Multi-carrier operation requires no other adjustment.
Carrier number zero is on the TX frequency displayed by qradiolink (baseband),
while carriers 1, 2, and 3 will be 25, 50 and 75 kHz above the carrier 0 frequency
if the channel separation is configured to be 25 kHz.
Carriers 4, 5 and 6 are 25, 50 and 75 kHz below the frequency displayed by qradiolink.
For example, a split of 3000 is a positive TX offset of 3 MHz for channel zero,
while the other channels will still be offset 3 MHz between RX and TX but have
a 25 kHz separation between each other. When starting MMDVM,
channel number 1 will be on baseband, channels 2, 3 and 4 will have positive
offsets while channels 5, 6 and 7 will be below baseband (main) frequency
with negative offsets

Example:
<pre>
  Channel 6     Channel 5   **Channel 1(main)** Channel 2    Channel 3     Channel 4
  433.8500       433.8750      433.9000         433.9250      433.950       433.9750
       |            |             |               |             |              |
       |            |             |               |             |              |
       |            |             |               |             |              |
-------|------------|-------------|---------------|-------------|--------------|---------
</pre>

            **Displayed frequency 433.9000**


* Set TX and RX gain as desired. It is recommended to start with a lower RX gain if not using a duplexer filter.

* The sample rate should not be configured and is fixed regardless what is configured in the UI.

* Enable the Duplex button (MMDVM modes are only functional in duplex mode).

* IMPORTANT NOTE: the demodulator offset has to be set such that all received channels are located inside the sampling window but channel zero is slightly shifted away from DC. A negative value about 12.5 kHz below the center frequency can be used, or about 12.5 kHz above the SDR center frequency. This is especially important in the **MMDVM multi** mode.

* Choose as operating mode either **MMDVM** or **MMDVM multi** for both TX and RX

* Click save to write the settings in the config file.

* Close the application, all the settings will be saved in the config file

* If you are unable to use the GUI all the settings can be instead directly written in the config file which is located in:
**~/.config/qradiolink/qradiolink.cfg**

* An example configuration file for MMDVM multicarrier (mode 36), qradiolink_config_example_mmdvm.cfg is included in this directory. Edit according to examples above and copy it to **~/.config/qradiolink/qradiolink.cfg**

* There is now a new setting in the config file: **burst_delay_msec** which controls how far in the future DMR timeslots are transmitted. This delay can be set to between 10 and 300 milliseconds, depending on the CPU on which qradiolink runs. Generally, the more powerful the CPU, the lower this setting can be set. The default value is 60 msec, but this value will generally need to be changed as it depends on the processing power of the platform.

* Finally, start the application from the CLI using the **--mmdvm** switch:
<pre>
./qradiolink --mmdvm
</pre>
If everything is configured ok, you should see the SDR initialization sequence and then no further messages. If the SDR FIFO runs too low or too high, or if the SDR drops sample packets, you will see a log message in the console.

* Due to some current implementation issues, it is not possible to operate in the MMDVM modes while using the graphical interface.

* Control of the transceiver can be performed while running by using telnet on local port 4939 via the CLI interface. For example:
<pre>
rlwrap telnet localhost 4939
qradiolink> rxfreq
Current RX frequency is 430900000.
qradiolink> tunerx 430899800
Tuning receiver to 430,899,800 Hz
qradiolink> rxgain
Current RX gain is 32.
qradiolink> setrxgain 35
Setting RX gain value to 35
</pre>

* When operating in MMDVM mode (duplex TX/RX), tuning the RX frequency will automatically tune the TX frequency keeping the defined TX offset (split).

* Keep in mind that SDR devices can have thermal frequency drift, so if the clock is not disciplined by an external precise source you may need to tune the receiver as the device warms and cools. MMDVM demodulation is relatively sensitive to being offset in frequency, so the first thing to check if you observe a high BER is the RX frequency which can be a few hundreds of Hz above or below the optimum value.
---


### 3. MMDVMHost
----

* Configure MMDVM.ini first. If using in multi-channel mode, create multiple files, for example MMDVM1.ini, MMDVM2.ini, MMDVM3.ini

* Each MMDVM.ini has to contain a unique identifier for the virtual pts. In single channel mode the name will always be ttyMMDVM0, otherwise it will be numbered from 1 to 7 depending on the id given after the **-c** switch. The path of the symlink will be located in the MMDVM-SDR build directory if started from there.

* Note that RX and TX levels are important and may need manual tweaking. Particularly, YSF mode requires much lower RX levels than DMR mode.

* Example config for DMR below:
<pre>
[General]
Callsign=YOUR_CALLSIGN
Id=YOUR_DMR_ID
Timeout=3600
Duplex=1
ModeHang=360
RFModeHang=360
NetModeHang=360
Display=None
Daemon=0

[Info]
RXFrequency=431500000
TXFrequency=433500000
Power=1
# The following lines are only needed if a direct connection to a DMR master is being used
Latitude=43.000
Longitude=22.000
Height=1
Location="My location"
Description="hotspot"
URL=dstar.su

[Log]
# Logging levels, 0=No logging
DisplayLevel=1
FileLevel=1
FilePath=.
FileRoot=MMDVM
FileRotate=1

[CW Id]
Enable=0
Time=10
Callsign=YOUR_CALLSIGN

[DMR Id Lookup]
File=DMRIds.dat
Time=24

[NXDN Id Lookup]
File=NXDN.csv
Time=24

[Modem]
Port=/home/user1/MMDVM-SDR/build/ttyMMDVM1
# Above value is not used anymore starting with the new version of MMDVMHost
# Use this value below instead
UARTPort=/home/user1/MMDVM-SDR/build/ttyMMDVM1
Protocol=uart
# Address=0x22
TXInvert=1
RXInvert=1
PTTInvert=0
TXDelay=0
RXOffset=0
TXOffset=0
DMRDelay=90
RXLevel=70
TXLevel=100
RXDCOffset=0
TXDCOffset=0
RFLevel=100
# CWIdTXLevel=50
# D-StarTXLevel=50
# DMRTXLevel=50
# YSFTXLevel=50
# P25TXLevel=50
# NXDNTXLevel=50
# POCSAGTXLevel=50
# FMTXLevel=50
RSSIMappingFile=RSSI.dat
UseCOSAsLockout=0
Trace=0
Debug=3

[Transparent Data]
Enable=0
RemoteAddress=127.0.0.1
RemotePort=40094
LocalPort=40095
# SendFrameType=0

[UMP]
Enable=0
# Port=\\.\COM4
Port=/dev/ttyACM1

[D-Star]
Enable=0
Module=C
SelfOnly=0
AckReply=1
AckTime=750
AckMessage=0
ErrorReply=1
RemoteGateway=0
# ModeHang=10
WhiteList=

[DMR]
Enable=1
# These 4 below settings are only relevant for tier III trunked radio mode
# Trunking=1
# SystemCode=9
# RegistrationRequired=1
# ControlChannel=1 # set this to 1 only for the first instance of MMDVMHost on the control channel
Beacons=1
BeaconInterval=30
BeaconDuration=10
ColorCode=1
#SelfOnly=0
#EmbeddedLCOnly=0
DumpTAData=0
Id=YOUR_DMR_ID
CallHang=7
TXHang=30
ModeHang=60
# OVCM Values, 0=off, 1=rx_on, 2=tx_on, 3=both_on, 4=force off
# OVCM=0

[System Fusion]
Enable=0
LowDeviation=0
SelfOnly=0
TXHang=4
RemoteGateway=0
# ModeHang=10

[P25]
Enable=0
NAC=293
SelfOnly=0
OverrideUIDCheck=0
RemoteGateway=0
TXHang=5
# ModeHang=10

[NXDN]
Enable=0
RAN=1
SelfOnly=0
RemoteGateway=0
TXHang=5
# ModeHang=10

[POCSAG]
Enable=0
Frequency=439987500

[FM]
Enable=0
Callsign=YOUR_CALLSIGN
CallsignSpeed=20
CallsignFrequency=1000
CallsignTime=10
CallsignHoldoff=0
CallsignHighLevel=50
CallsignLowLevel=20
CallsignAtStart=1
CallsignAtEnd=1
CallsignAtLatch=0
RFAck=K
ExtAck=N
AckSpeed=20
AckFrequency=1750
AckMinTime=4
AckDelay=1000
AckLevel=50
# Timeout=180
TimeoutLevel=80
CTCSSFrequency=88.4
CTCSSThreshold=30
# CTCSSHighThreshold=30
# CTCSSLowThreshold=20
CTCSSLevel=20
KerchunkTime=0
HangTime=7
AccessMode=1
COSInvert=0
RFAudioBoost=1
MaxDevLevel=90
ExtAudioBoost=1

[D-Star Network]
Enable=0
GatewayAddress=127.0.0.1
GatewayPort=20010
LocalPort=20011
# ModeHang=3
Debug=0

[DMR Network]
Enable=1
# Type may be either 'Direct' or 'Gateway'. When Direct you must provide the Master's
# address as well as the Password, and for DMR+, Options also.
Type=Direct
Address=master.dmr.server.address
Port=62031
Local=62033
Jitter=300
Password=password
# Options=
Slot1=1
Slot2=1
# ModeHang=3
Debug=0

[System Fusion Network]
Enable=0
LocalAddress=127.0.0.1
LocalPort=3200
GatewayAddress=127.0.0.1
GatewayPort=4200
# ModeHang=3
Debug=0

[P25 Network]
Enable=0
GatewayAddress=127.0.0.1
GatewayPort=42020
LocalPort=32010
# ModeHang=3
Debug=0

[NXDN Network]
Protocol=Icom
Enable=0
LocalAddress=127.0.0.1
LocalPort=14021
GatewayAddress=127.0.0.1
GatewayPort=14020
# ModeHang=3
Debug=0

[POCSAG Network]
Enable=0
LocalAddress=127.0.0.1
LocalPort=3800
GatewayAddress=127.0.0.1
GatewayPort=4800
# ModeHang=3
Debug=0

[TFT Serial]
# Port=modem
Port=/dev/ttyAMA0
Brightness=50

[HD44780]
Rows=2
Columns=16

# For basic HD44780 displays (4-bit connection)
# rs, strb, d0, d1, d2, d3
Pins=11,10,0,1,2,3

# Device address for I2C
I2CAddress=0x20

# PWM backlight
PWM=0
PWMPin=21
PWMBright=100
PWMDim=16

DisplayClock=1
UTC=0

[Nextion]
# Port=modem
Port=/dev/ttyAMA0
Brightness=50
DisplayClock=1
UTC=0
#Screen Layout: 0=G4KLX 2=ON7LDS
ScreenLayout=2
IdleBrightness=20

[OLED]
Type=3
Brightness=0
Invert=0
Scroll=1
Rotate=0
Cast=0
LogoScreensaver=1

[LCDproc]
Address=localhost
Port=13666
#LocalPort=13667
DimOnIdle=0
DisplayClock=1
UTC=0

[Lock File]
Enable=0
File=/tmp/MMDVM_Active.lck

[Remote Control]
Enable=0
Address=127.0.0.1
Port=7642
</pre>

* For single channel mode, start MMDVMHost using the desired name of the config file. If using multi-channel mode,
start multiple instances of MMDVMHost from different terminals using the corresponding ini config files. Example for single channel mode:
<pre>
./MMDVMHost MMDVM.ini
</pre>

* Example for multi-channel mode:
<pre>
./MMDVMHost MMDVM1.ini
./MMDVMHost MMDVM2.ini
./MMDVMHost MMDVM3.ini
</pre>

* You can configure different networks, modes or combination of talkgroups for each channel.

* Due to a limitation in the way multicarrier mode is implemented, channel 1 in MMDVM (or channel 0 in qradiolink) must always be assigned to a DMR channel, or alternatively must be left unassigned (no traffic), otherwise timing synchronziation will be affected.

* The most important options in the MMDVM.ini files are located in the [Modem] section.
You **must** enable **TXInvert** and **RXInvert**, and also adjust RXLevel to a value where the BER is minimal, 
generally between 1 and 70. TXLevel can be set to values between 80 and 100 in most cases everywhere. System Fusion optimum RXLevel is lower than DMR level. A value of 1 can work best in most cases for YSF. 
----
