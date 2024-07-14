/* -*- c++ -*- */
/*
 * Copyright 2018 Lime Microsystems info@limemicro.com
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "device_handler.h"
#include "logging.h"

#include <gnuradio/logger.h>

#include "limesuiteng/DeviceRegistry.h"
#include "limesuiteng/VersionInfo.h"

#include <array>
#include <cmath>
#include <stdexcept>

using namespace std::literals::string_literals;

device_handler::device_handler()
{
    gr::configure_default_loggers(d_logger, d_debug_logger, "device_handler");
    set_limesuite_logger();
}

device_handler::~device_handler() {}

void device_handler::error(int device_number)
{
    GR_LOG_WARN(d_logger, lime::GetLastErrorMessageCString());

    if (device_vector[device_number].address != nullptr)
        close_all_devices();
}

lime::SDRDevice* device_handler::get_device(int device_number)
{
    return device_vector.at(device_number).address;
}

lime::StreamConfig& device_handler::get_stream_config(int device_number)
{
    return device_vector.at(device_number).stream_config;
}

int device_handler::open_device(std::string& serial)
{
    set_limesuite_logger();

    int device_number = 0;
    std::string search_name;

    // Print device and library information only once
    if (list_read == false) {
        GR_LOG_INFO(d_logger, "##################");
        GR_LOG_INFO(
            d_logger,
            fmt::format("Lime Suite NG version: {:s}", lime::GetLibraryVersion()));
        GR_LOG_INFO(d_logger, fmt::format("gr-limesdr version: {:s}", GR_LIMESDR_VER));
        GR_LOG_INFO(d_logger, "##################");

        found_devices = lime::DeviceRegistry::enumerate();
        if (found_devices.empty()) {
            throw std::runtime_error(
                "device_handler::open_device(): No Lime devices found.");
        }
        GR_LOG_INFO(d_logger, "Device list:");

        for (std::size_t i = 0; i < found_devices.size(); i++) {
            GR_LOG_INFO(d_logger,
                        fmt::format("Device {:d}: {:s}", i, found_devices[i].ToString()));
        }
        device_vector.resize(found_devices.size());
        GR_LOG_INFO(d_logger, "##################");
        list_read = true;
    }

    if (serial.empty()) {
        GR_LOG_INFO(d_logger,
                    "device_handler::open_device(): no serial number. Using first device "
                    "in the list.");
        GR_LOG_INFO(d_logger,
                    "Use \"limeDevice\" in terminal to find preferred device serial.");

        device_number = 0;
        serial = found_devices[0].serial;
    } else {
        auto device_iter = std::find_if(found_devices.begin(),
                                        found_devices.end(),
                                        [&](const lime::DeviceHandle device_handle) {
                                            return serial == device_handle.serial;
                                        });
        if (device_iter == found_devices.end()) {
            close_all_devices();
            throw std::invalid_argument("Unable to find LMS device with serial " +
                                        serial);
        }

        device_number = device_iter - found_devices.begin();
        serial = device_iter->serial;
    }

    // If device slot is empty, open and initialize device
    if (device_vector[device_number].address == nullptr) {
        auto sdr_device = lime::DeviceRegistry::makeDevice(found_devices[device_number]);
        device_vector[device_number].address = sdr_device;

        if (device_vector[device_number].address == nullptr) {
            throw std::runtime_error(
                "device_handler::open_device(): failed to open device " + serial);
        }

        sdr_device->Init();

        const auto& descriptor = sdr_device->GetDescriptor();

        GR_LOG_INFO(d_logger,
                    fmt::format("Using device: {:s} ({:s}) GW: {:s} FW: {:s}",
                                descriptor.name,
                                serial,
                                descriptor.gatewareVersion,
                                descriptor.firmwareVersion));
        GR_LOG_INFO(d_logger, "##################");
    }
    // If device is open do nothing
    else {
        GR_LOG_INFO(
            d_logger,
            fmt::format("Previously connected device serial {:s} from the list is used.",
                        found_devices[device_number].serial));
        GR_LOG_INFO(d_logger, "##################");
    }
    set_limesuite_logger();

    return device_number; // return device number to identify
                          // device_vector[device_number].address connection in other
                          // functions
}

void device_handler::close_device(int device_number, int block_type)
{
    // If two blocks used switch one block flag and let other block finish work
    // Switch flag when closing device
    switch (block_type) {
    case 1:
        device_vector[device_number].source_flag = false;
        break;
    case 2:
        device_vector[device_number].sink_flag = false;
        break;
    }

    // Check if other block finished and close device
    if (device_vector[device_number].source_flag ||
        device_vector[device_number].sink_flag ||
        device_vector[device_number].address == nullptr) {
        return;
    }

    GR_LOG_INFO(d_logger, "##################");
    auto status = device_vector[device_number].address->Reset();
    if (status != lime::OpStatus::Success)
        error(device_number);

    lime::DeviceRegistry::freeDevice(device_vector[device_number].address);

    GR_LOG_INFO(d_logger,
                fmt::format("device_handler::close_device(): disconnected from "
                            "device number {:d}.",
                            device_number));
    device_vector[device_number].address = nullptr;
    GR_LOG_INFO(d_logger, "##################");
}

void device_handler::close_all_devices()
{
    if (close_flag) {
        return;
    }

    for (std::size_t i = 0; i < device_vector.size(); i++) {
        if (device_vector[i].address != nullptr) {
            device_vector[i].address->Reset();
            lime::DeviceRegistry::freeDevice(device_vector[i].address);

            device_vector[i] = device();
        }
    }
    close_flag = true;
}

void device_handler::check_blocks(int device_number,
                                  int block_type,
                                  int channel_mode,
                                  const std::string& filename)
{
    // Get each block settings
    switch (block_type) {
    case 1: // Source block
        if (device_vector[device_number].source_flag == true) {
            close_all_devices();
            throw std::invalid_argument(
                "ERROR: device_handler::check_blocks(): only one LimeSuite "
                "Source (RX) block is allowed per device.");
        } else {
            device_vector[device_number].source_flag = true;
            device_vector[device_number].source_channel_mode = channel_mode;
            device_vector[device_number].source_filename = filename;
        }
        break;

    case 2: // Sink block
        if (device_vector[device_number].sink_flag == true) {
            throw std::invalid_argument(
                "ERROR: device_handler::check_blocks(): only one LimeSuite "
                "Sink (TX) block is allowed per device.");
        } else {
            device_vector[device_number].sink_flag = true;
            device_vector[device_number].sink_channel_mode = channel_mode;
            device_vector[device_number].sink_filename = filename;
        }
        break;

    default:
        close_all_devices();
        throw std::invalid_argument(
            fmt::format("device_handler::check_blocks(): internal error, incorrect "
                        "block_type value {:d}.",
                        block_type));
    }

    // Check block settings which must match
    if (!device_vector[device_number].source_flag ||
        !device_vector[device_number].sink_flag) {
        return;
    }

    // Chip_mode must match in blocks with the same serial
    if (device_vector[device_number].source_channel_mode !=
        device_vector[device_number].sink_channel_mode) {
        close_all_devices();
        throw std::invalid_argument(
            fmt::format("device_handler::check_blocks(): channel mismatch in LimeSuite "
                        "Source (RX) ({:d}) and LimeSuite Sink (TX) ({:d})",
                        device_vector[device_number].source_channel_mode,
                        device_vector[device_number].sink_channel_mode));
    }

    // When file_switch is 1 check filename match throughout the blocks with the same
    // serial
    if (device_vector[device_number].source_filename !=
        device_vector[device_number].sink_filename) {
        close_all_devices();
        throw std::invalid_argument(
            fmt::format("device_handler::check_blocks(): file must match in LimeSuite "
                        "Source (RX) ({:s}) and LimeSuite Sink (TX) ({:s})",
                        device_vector[device_number].source_filename,
                        device_vector[device_number].sink_filename));
    }
}

void device_handler::settings_from_file(int device_number,
                                        const std::string& filename,
                                        int* pAntenna_tx)
{
    auto& instance = device_handler::getInstance();
    const auto& device = instance.get_device(device_number);
    auto status = device->LoadConfig(0, filename);

    if (status != lime::OpStatus::Success)
        instance.error(device_number);

    // Set LimeSDR-Mini switches based on .ini file
    std::array<int, 2> antenna_rx = { 0, 0 };
    std::array<int, 2> antenna_tx = { 0, 0 };
    antenna_tx[0] = device->GetAntenna(0, lime::TRXDir::Tx, 0);
    /* Don't print error message for the mini board */
    suppress_limesuite_logging();
    antenna_tx[1] = device->GetAntenna(0, lime::TRXDir::Tx, 1);
    antenna_rx[1] = device->GetAntenna(0, lime::TRXDir::Rx, 1);

    // Restore our logging
    set_limesuite_logger();
    antenna_rx[0] = device->GetAntenna(0, lime::TRXDir::Rx, 0);

    if (pAntenna_tx != nullptr) {
        pAntenna_tx[0] = antenna_tx[0];
        pAntenna_tx[1] = antenna_tx[1];
    }

    device->SetAntenna(0, lime::TRXDir::Tx, 0, antenna_tx[0]);
    device->SetAntenna(0, lime::TRXDir::Rx, 0, antenna_rx[0]);
}

void device_handler::enable_channels(int device_number,
                                     int channel_mode,
                                     lime::TRXDir direction)
{
    GR_LOG_DEBUG(d_debug_logger, "device_handler::enable_channels(): ");
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (channel_mode < 2) {
        if (device->EnableChannel(0, direction, channel_mode, true) !=
            lime::OpStatus::Success) {
            instance.error(device_number);
        }
        GR_LOG_INFO(d_logger,
                    fmt::format("SISO CH{:d} set for device number {:d}.",
                                channel_mode,
                                device_number));

#ifdef ENABLE_RFE
        if (direction == lime::TRXDir::Tx)
            rfe_device.tx_channel = channel_mode;
        else
            rfe_device.rx_channel = channel_mode;

        if (rfe_device.rfe_dev) {
            update_rfe_channels();
        }
#endif
    } else if (channel_mode == 2) {
        if (device->EnableChannel(0, direction, 0, true) != lime::OpStatus::Success)
            instance.error(device_number);
        if (device->EnableChannel(0, direction, 1, true) != lime::OpStatus::Success)
            instance.error(device_number);

        GR_LOG_INFO(d_logger,
                    fmt::format("MIMO mode set for device number {:d}.", device_number));
    }
}

void device_handler::set_samp_rate(int device_number, double& rate)
{
    GR_LOG_DEBUG(d_debug_logger, "device_handler::set_samp_rate(): ");
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (device->SetSampleRate(0, lime::TRXDir::Rx, 1, rate, 0) != lime::OpStatus::Success)
        instance.error(device_number);

    double host_value = device->GetSampleRate(0, lime::TRXDir::Rx, 1);

    GR_LOG_INFO(d_logger,
                fmt::format("Set sampling rate: {:f} MS/s.", (host_value / 1e6)));
    rate = host_value; // Get the real rate back
}

void device_handler::set_oversampling(int device_number, int oversample)
{
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    switch (oversample) {
    case 0:
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32: {
        GR_LOG_DEBUG(d_debug_logger, "device_handler::set_oversampling(): ");
        double host_value = device->GetSampleRate(0, lime::TRXDir::Rx, 1);

        if (device->SetSampleRate(0, lime::TRXDir::Rx, 1, host_value, oversample) !=
            lime::OpStatus::Success) {
            instance.error(device_number);
        }

        GR_LOG_INFO(d_logger, fmt::format("Set oversampling: {:d}.", oversample));
    } break;
    default:
        close_all_devices();
        throw std::invalid_argument(
            "device_handler::set_oversampling(): valid oversample values are: "
            "0,1,2,4,8,16,32.");
    }
}

double device_handler::set_rf_freq(int device_number,
                                   lime::TRXDir direction,
                                   int channel,
                                   float rf_freq)
{
    double value = 0;
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (rf_freq <= 0) {
        close_all_devices();
        throw std::invalid_argument(
            "device_handler::set_rf_freq(): rf_freq must be more than 0 Hz.");
    }

    GR_LOG_DEBUG(d_debug_logger, "device_handler::set_rf_freq(): ");
    if (device->SetFrequency(0, direction, channel, rf_freq) != lime::OpStatus::Success) {
        instance.error(device_number);
    }

    value = device->GetFrequency(0, direction, channel);

    const std::array<std::string, 2> s_dir = { "RX"s, "TX"s };
    GR_LOG_INFO(d_logger,
                fmt::format("RF frequency set [{:s}]: {:f} MHz.",
                            s_dir[direction == lime::TRXDir::Tx ? 1 : 0],
                            (value / 1e6)));

    return value;
}

void device_handler::calibrate(int device_number,
                               lime::TRXDir direction,
                               int channel,
                               double bandwidth)
{
    GR_LOG_DEBUG(d_debug_logger, "device_handler::calibrate(): ");
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (device->Calibrate(0, direction, channel, bandwidth) != lime::OpStatus::Success) {
        instance.error(device_number);
    }
}

void device_handler::set_antenna(int device_number,
                                 int channel,
                                 lime::TRXDir direction,
                                 int antenna)
{
    GR_LOG_DEBUG(d_debug_logger, "device_handler::set_antenna(): ");
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (device->SetAntenna(0, direction, channel, antenna) != lime::OpStatus::Success)
        instance.error(device_number);

    int antenna_value = device->GetAntenna(0, direction, channel);

    const auto s_antenna = device->GetDescriptor().rfSOC.at(0).pathNames;
    const std::array<std::string, 2> s_dir = { "RX"s, "TX"s };
    GR_LOG_INFO(d_logger,
                fmt::format("CH{:d} antenna set [{:s}]: {:s}.",
                            channel,
                            s_dir.at(direction == lime::TRXDir::Tx ? 1 : 0),
                            s_antenna.at(direction).at(antenna_value)));
}

double device_handler::set_analog_filter(int device_number,
                                         lime::TRXDir direction,
                                         int channel,
                                         double analog_bandw)
{
    double analog_value = 0;
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (channel < 0 || channel > 1) {
        close_all_devices();
        throw std::invalid_argument(
            "device_handler::set_analog_filter(): channel must be 0 or 1.");
    }

    GR_LOG_DEBUG(d_debug_logger, "device_handler::set_analog_filter(): ");
    if (device->SetLowPassFilter(0, direction, channel, analog_bandw) !=
        lime::OpStatus::Success) {
        instance.error(device_number);
    }

    analog_value = device->GetLowPassFilter(0, direction, channel);

    return analog_value;
}

double device_handler::set_digital_filter(int device_number,
                                          lime::TRXDir direction,
                                          int channel,
                                          double digital_bandw)
{
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (channel < 0 || channel > 1) {
        close_all_devices();
        throw std::invalid_argument(
            "device_handler::set_digital_filter(): channel must be 0 or 1.");
    }

    bool enable = (digital_bandw > 0) ? true : false;
    GR_LOG_DEBUG(d_debug_logger, "device_handler::set_digital_filter(): ");
    if (device->ConfigureGFIR(0, direction, channel, { enable, digital_bandw }) !=
        lime::OpStatus::Success) {
        instance.error(device_number);
    }

    const std::array<std::string, 2> s_dir = { "RX"s, "TX"s };
    const std::string msg_start =
        fmt::format("Digital filter CH{:d} [{:s}]",
                    channel,
                    s_dir[direction == lime::TRXDir::Tx ? 1 : 0]);

    if (enable) {
        GR_LOG_INFO(d_logger,
                    fmt::format("{:s} set: {:f}", msg_start, digital_bandw / 1e6));
    } else {
        GR_LOG_INFO(d_logger, fmt::format("{:s} disabled.", msg_start));
    }

    return digital_bandw;
}

unsigned device_handler::set_gain(int device_number,
                                  lime::TRXDir direction,
                                  int channel,
                                  unsigned gain_dB)
{
    unsigned gain_value = 0;
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (gain_dB < 0 || gain_dB > 73) {
        close_all_devices();
        throw std::invalid_argument("device_handler::set_gain(): valid range [0, 73]");
    }

    GR_LOG_DEBUG(d_debug_logger, "device_handler::set_gain(): ");

    if (device->SetGain(0, direction, channel, lime::eGainTypes::UNKNOWN, gain_dB) !=
        lime::OpStatus::Success) {
        instance.error(device_number);
    }

    const std::array<std::string, 2> s_dir = { "RX"s, "TX"s };

    double gain_double = 0.0;
    if (device->GetGain(0, direction, channel, lime::eGainTypes::UNKNOWN, gain_double) !=
        lime::OpStatus::Success) {
        instance.error(device_number);
    }

    gain_value = std::lround(gain_double) + 12;
    GR_LOG_INFO(d_logger,
                fmt::format("CH{:d} gain set [{:s}]: {:d}.",
                            channel,
                            s_dir[direction == lime::TRXDir::Tx ? 1 : 0],
                            gain_value));

    return gain_value;
}

void device_handler::set_nco(int device_number,
                             lime::TRXDir direction,
                             int channel,
                             float nco_freq)
{
    const std::array<std::string, 2> s_dir = { "RX"s, "TX"s };
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    GR_LOG_DEBUG(d_debug_logger, "device_handler::set_nco(): ");
    if (nco_freq == 0) {
        if (device->SetNCOIndex(0, direction, channel, 0, false) !=
            lime::OpStatus::Success) {
            instance.error(device_number);
        }

        GR_LOG_INFO(d_logger,
                    fmt::format("NCO [{:s}] CH{:d} gain disabled.",
                                s_dir[direction == lime::TRXDir::Tx ? 1 : 0],
                                channel));
        return;
    }

    int cmix_mode = 0;

    if (nco_freq > 0)
        cmix_mode = 0;
    else if (nco_freq < 0)
        cmix_mode = 1;

    for (int i = 0; i < 16; ++i) {
        if (device->SetNCOFrequency(0, direction, channel, i, nco_freq) !=
            lime::OpStatus::Success) {
            instance.error(device_number);
        }
    }

    if (device->SetNCOIndex(0, direction, channel, 0, cmix_mode) !=
        lime::OpStatus::Success) {
        instance.error(device_number);
    }

    const std::array<std::string, 2> s_cmix = { "UPCONVERT"s, "DOWNCONVERT"s };

    double pho_value_out{ NAN };
    const auto freq_value_out =
        device->GetNCOFrequency(0, direction, channel, 0, pho_value_out);

    GR_LOG_INFO(d_logger,
                fmt::format("NCO [{:s}] CH{:d}: {:f} MHz ({:f} deg.)({:s}).",
                            s_dir[direction == lime::TRXDir::Tx ? 1 : 0],
                            channel,
                            freq_value_out / 1e6,
                            pho_value_out,
                            s_cmix[cmix_mode]));
}

void device_handler::disable_DC_corrections(int device_number)
{
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (device->SetParameter(0, 0, "DC_BYP_RXTSP", 1) != lime::OpStatus::Success)
        instance.error(device_number);

    if (device->SetParameter(0, 0, "DCLOOP_STOP", 1) != lime::OpStatus::Success)
        instance.error(device_number);
}

void device_handler::set_tcxo_dac(int device_number, uint16_t dacVal)
{
    GR_LOG_DEBUG(d_debug_logger, "device_handler::set_txco_dac(): ");
    double dac_value = dacVal;
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    std::vector<lime::CustomParameterIO> parameter{ { 0, dac_value, ""s } };

    if (device->CustomParameterWrite(parameter) != lime::OpStatus::Success)
        instance.error(device_number);

    if (device->CustomParameterRead(parameter) != lime::OpStatus::Success)
        instance.error(device_number);

    GR_LOG_INFO(d_logger,
                fmt::format("VCTCXO DAC value set: {:d}", parameter.at(0).value));
}

#ifdef ENABLE_RFE
void device_handler::set_rfe_device(rfe_dev_t* rfe_dev) { rfe_device.rfe_dev = rfe_dev; }

void device_handler::update_rfe_channels()
{
    if (!rfe_device.rfe_dev) {
        throw std::runtime_error(
            "device_handler::update_rfe_channels(): no assigned RFE device");
    }

    GR_LOG_DEBUG(d_debug_logger, "device_handler::update_rfe_channels(): ");
    if (RFE_AssignSDRChannels(
            rfe_device.rfe_dev, rfe_device.rx_channel, rfe_device.tx_channel) != 0) {
        throw std::runtime_error("Failed to assign SDR channels");
    }
    GR_LOG_INFO(d_logger,
                fmt::format("RFE RX channel: {:d} TX channel: {:d}",
                            rfe_device.rx_channel,
                            rfe_device.tx_channel));
}
#endif

void device_handler::write_lms_reg(int device_number, uint32_t address, uint16_t val)
{
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (device->WriteRegister(0, address, val) != lime::OpStatus::Success)
        instance.error(device_number);
}

void device_handler::set_gpio_dir(int device_number, uint8_t dir)
{
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (device->GPIODirWrite(&dir, 1) != lime::OpStatus::Success)
        instance.error(device_number);
}

void device_handler::write_gpio(int device_number, uint8_t out)
{
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    if (device->GPIOWrite(&out, 1) != lime::OpStatus::Success)
        instance.error(device_number);
}

uint8_t device_handler::read_gpio(int device_number)
{
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);
    uint8_t res = 0;

    if (device->GPIORead(&res, 1) != lime::OpStatus::Success)
        instance.error(device_number);

    return res;
}
