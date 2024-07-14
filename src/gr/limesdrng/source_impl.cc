/* -*- c++ -*- */
/*
 * Copyright 2019 Lime Microsystems.
 *
 * This is free software; you can redistribute it and/or modify
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "logging.h"
#include "source_impl.h"
#include <gnuradio/io_signature.h>

#include <stdexcept>

namespace gr {
namespace limesdr {

source::sptr source::make(std::string serial,
                          int channel_mode,
                          const std::string& filename,
                          bool align_ch_phase)
{
    return gnuradio::get_initial_sptr(
        new source_impl(serial, channel_mode, filename, align_ch_phase));
}

/*
 * The private constructor
 */
// gr::sync_block("source", gr::io_signature::make(0, 0, 0), gr::io_signature::make(1, 1,
// sizeof(gr_complex)))

source_impl::source_impl(std::string serial,
                         int channel_mode,
                         const std::string& filename,
                         bool align_ch_phase)
    : gr::sync_block(fmt::format("source {:s}", serial),
                     gr::io_signature::make(0, 0, 0),
                     args_to_io_signature(channel_mode))
{
    set_limesuite_logger();

    // 1. Store private variables upon implementation to protect from changing
    // them later
    stored.serial = serial;
    stored.channel_mode = channel_mode;
    stored.align = align_ch_phase ? (1 << 16) : 0;

    if (stored.channel_mode < 0 || stored.channel_mode > 2) {
        throw std::invalid_argument(
            "source_impl::source_impl(): Channel must be A(0), B(1), or (A+B) MIMO(2)");
    }

    auto& instance = device_handler::getInstance();

    // 2. Open device if not opened
    stored.device_number = instance.open_device(stored.serial);
    // 3. Check where to load settings from (file or block)
    if (!filename.empty()) {
        instance.settings_from_file(stored.device_number, filename, nullptr);
        instance.check_blocks(
            stored.device_number, source_block, stored.channel_mode, filename);
    } else {
        // 4. Check how many blocks were used and check values between blocks
        instance.check_blocks(
            stored.device_number, source_block, stored.channel_mode, "");

        // 5. Enable required channel/s
        instance.enable_channels(
            stored.device_number, stored.channel_mode, lime::TRXDir::Rx);
    }
}

/*
 * Our virtual destructor.
 */
source_impl::~source_impl()
{
    auto& instance = device_handler::getInstance();

    instance.get_device(stored.device_number)->StreamStop(0);
    instance.close_device(stored.device_number, source_block);
}

bool source_impl::start(void)
{
    auto& instance = device_handler::getInstance();

    std::unique_lock<std::recursive_mutex> lock(instance.block_mutex);
    // Initialize and start stream for channel 0 (if channel_mode is SISO)
    if (stored.channel_mode < 2) // If SISO configure prefered channel
    {
        init_stream(stored.device_number, stored.channel_mode);
    }

    // Initialize and start stream for channels 0 & 1 (if channel_mode is MIMO)
    else if (stored.channel_mode == 2) {

        init_stream(stored.device_number, 0);
        init_stream(stored.device_number, 1);
    }

    instance.get_device(stored.device_number)->StreamStart(0);

    std::unique_lock<std::recursive_mutex> unlock(instance.block_mutex);

    if (stream_analyzer) {
        t1 = std::chrono::high_resolution_clock::now();
        t2 = t1;
    }

    add_tag = true;

    return true;
}

bool source_impl::stop(void)
{
    auto& instance = device_handler::getInstance();

    std::unique_lock<std::recursive_mutex> lock(instance.block_mutex);

    instance.get_device(stored.device_number)->StreamStop(0);

    std::unique_lock<std::recursive_mutex> unlock(instance.block_mutex);
    return true;
}

int source_impl::work(int noutput_items,
                      gr_vector_const_void_star& input_items,
                      gr_vector_void_star& output_items)
{
    int ret = 0;

    // Receive stream for channel 0 (if channel_mode is SISO)
    lime::StreamStats status;
    lime::StreamMeta rx_metadata{};

    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(stored.device_number);

    switch (instance.get_stream_config(stored.device_number).format) {
    case lime::DataFormat::F32:
        ret = device->StreamRx(
            0,
            reinterpret_cast<lime::complex32f_t* const*>(output_items.data()),
            noutput_items,
            &rx_metadata);
        break;
    case lime::DataFormat::I16:
        ret = device->StreamRx(
            0,
            reinterpret_cast<lime::complex16_t* const*>(output_items.data()),
            noutput_items,
            &rx_metadata);
        break;
    case lime::DataFormat::I12:
        ret = device->StreamRx(
            0,
            reinterpret_cast<lime::complex12_t* const*>(output_items.data()),
            noutput_items,
            &rx_metadata);
        break;

    default:
        throw std::logic_error("Unsupported format");
        break;
    }

    if (ret < 0) {
        return 0;
    }

    device->StreamStatus(0, &status, nullptr);

    if (add_tag || status.loss > 0) {
        pktLoss += status.loss;
        add_tag = false;
        add_time_tag(0, rx_metadata);

        if (stored.channel_mode == 2) {
            add_time_tag(1, rx_metadata);
        }
    }
    // Print stream stats to debug
    if (stream_analyzer == true) {
        print_stream_stats(status);
    }

    produce(0, ret);
    if (stored.channel_mode == 2) {
        produce(1, ret);
    }

    return WORK_CALLED_PRODUCE;
}

// Setup stream
void source_impl::init_stream(int device_number, int channel)
{
    auto& instance = device_handler::getInstance();

    auto& config = instance.get_stream_config(device_number);
    config.channels.at(lime::TRXDir::Rx).push_back(channel);
    config.bufferSize = (stored.FIFO_size == 0) ? static_cast<int>(stored.samp_rate) / 10
                                                : stored.FIFO_size;
    config.format = lime::DataFormat::F32;
    config.linkFormat = lime::DataFormat::I16;

    if (instance.get_device(device_number)->StreamSetup(config, 0) !=
        lime::OpStatus::Success) {
        instance.error(device_number);
    }

    GR_LOG_INFO(
        d_logger,
        fmt::format(
            "init_stream: source channel {:d} (device nr. {:d}) stream setup done.",
            channel,
            device_number));
}

// Print stream status
void source_impl::print_stream_stats(lime::StreamStats status)
{
    t2 = std::chrono::high_resolution_clock::now();
    auto timePeriod =
        std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    if (timePeriod < 1000) {
        return;
    }

    GR_LOG_INFO(d_logger,
                "---------------------------------------------------------------");
    GR_LOG_INFO(d_logger,
                fmt::format("RX |rate: {:f} MB/s |dropped packets: {:d} |FIFO: {:d}%",
                            (status.dataRate_Bps / 1e6),
                            pktLoss,
                            (100 * status.FIFO.ratio())));
    GR_LOG_INFO(d_logger,
                "---------------------------------------------------------------");
    pktLoss = 0;
    t1 = t2;
}

// Add rx_time tag to stream
void source_impl::add_time_tag(int channel, lime::StreamMeta meta)
{

    uint64_t u_rate = static_cast<uint64_t>(stored.samp_rate);
    double f_rate = stored.samp_rate - u_rate;
    uint64_t intpart = meta.timestamp / u_rate;
    double fracpart =
        (meta.timestamp - intpart * u_rate - intpart * f_rate) / stored.samp_rate;

    const pmt::pmt_t ID = pmt::string_to_symbol(stored.serial);
    const pmt::pmt_t t_val =
        pmt::make_tuple(pmt::from_uint64(intpart), pmt::from_double(fracpart));
    add_item_tag(channel, nitems_written(channel), TIME_TAG, t_val, ID);
}

// Return io_signature to manage module output count
// based on SISO (one output) and MIMO (two outputs) modes
inline gr::io_signature::sptr source_impl::args_to_io_signature(int channel_number)
{
    if (channel_number < 2) {
        return gr::io_signature::make(1, 1, sizeof(gr_complex));
    } else if (channel_number == 2) {
        return gr::io_signature::make(2, 2, sizeof(gr_complex));
    } else {
        throw std::invalid_argument(
            "source_impl::args_to_io_signature(): channel_number must be 0, 1 or 2.");
    }
}

double source_impl::set_center_freq(double freq, size_t chan)
{
    add_tag = true;
    return device_handler::getInstance().set_rf_freq(
        stored.device_number, lime::TRXDir::Rx, 0, freq);
}

void source_impl::set_nco(float nco_freq, int channel)
{
    device_handler::getInstance().set_nco(
        stored.device_number, lime::TRXDir::Rx, channel, nco_freq);
    add_tag = true;
}

void source_impl::set_antenna(int antenna, int channel)
{
    device_handler::getInstance().set_antenna(
        stored.device_number, channel, lime::TRXDir::Rx, antenna);
}

double source_impl::set_bandwidth(double analog_bandw, int channel)
{
    add_tag = true;
    return device_handler::getInstance().set_analog_filter(
        stored.device_number, lime::TRXDir::Rx, channel, analog_bandw);
}

void source_impl::set_digital_filter(double digital_bandw, int channel)
{
    device_handler::getInstance().set_digital_filter(
        stored.device_number, lime::TRXDir::Rx, channel, digital_bandw);
    add_tag = true;
}

unsigned source_impl::set_gain(unsigned gain_dB, int channel)
{
    return device_handler::getInstance().set_gain(
        stored.device_number, lime::TRXDir::Rx, channel, gain_dB);
}

void source_impl::calibrate(double bandw, int channel)
{
    device_handler::getInstance().calibrate(
        stored.device_number, lime::TRXDir::Rx, channel, bandw);
}

double source_impl::set_sample_rate(double rate)
{
    device_handler::getInstance().set_samp_rate(stored.device_number, rate);
    stored.samp_rate = rate;
    return rate;
}

void source_impl::set_buffer_size(uint32_t size) { stored.FIFO_size = size; }

void source_impl::set_oversampling(int oversample)
{
    device_handler::getInstance().set_oversampling(stored.device_number, oversample);
}

void source_impl::set_tcxo_dac(uint16_t dacVal)
{
    device_handler::getInstance().set_tcxo_dac(stored.device_number, dacVal);
}

void source_impl::write_lms_reg(uint32_t address, uint16_t val)
{
    device_handler::getInstance().write_lms_reg(stored.device_number, address, val);
}

void source_impl::set_gpio_dir(uint8_t dir)
{
    device_handler::getInstance().set_gpio_dir(stored.device_number, dir);
}

void source_impl::write_gpio(uint8_t out)
{
    device_handler::getInstance().write_gpio(stored.device_number, out);
}

uint8_t source_impl::read_gpio()
{
    return device_handler::getInstance().read_gpio(stored.device_number);
}

} /* namespace limesdr */
} /* namespace gr */
