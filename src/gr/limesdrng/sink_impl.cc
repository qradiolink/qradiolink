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
#include "sink_impl.h"
#include <gnuradio/io_signature.h>

#include <stdexcept>

namespace gr {
namespace limesdr {
sink::sptr sink::make(std::string serial,
                      int channel_mode,
                      const std::string& filename,
                      const std::string& length_tag_name)
{
    return gnuradio::get_initial_sptr(
        new sink_impl(serial, channel_mode, filename, length_tag_name));
}

sink_impl::sink_impl(std::string serial,
                     int channel_mode,
                     const std::string& filename,
                     const std::string& length_tag_name)
    : gr::sync_block(fmt::format("sink {:s}", serial),
                     args_to_io_signature(channel_mode),
                     gr::io_signature::make(0, 0, 0))
{
    set_limesuite_logger();

    LENGTH_TAG =
        length_tag_name.empty() ? pmt::PMT_NIL : pmt::string_to_symbol(length_tag_name);
    // 1. Store private variables upon implementation to protect from changing them later
    stored.serial = serial;
    stored.channel_mode = channel_mode;

    if (stored.channel_mode < 0 || stored.channel_mode > 2) {
        throw std::invalid_argument(
            "sink_impl::sink_impl(): Channel must be A(0), B(1), or (A+B) MIMO(2)");
    }

    auto& instance = device_handler::getInstance();

    // 2. Open device if not opened
    stored.device_number = instance.open_device(stored.serial);
    // 3. Check where to load settings from (file or block)
    if (!filename.empty()) {
        instance.settings_from_file(stored.device_number, filename, pa_path.data());
        instance.check_blocks(
            stored.device_number, sink_block, stored.channel_mode, filename);
    } else {
        // 4. Check how many blocks were used and check values between blocks
        instance.check_blocks(stored.device_number, sink_block, stored.channel_mode, "");

        // 5. Enable required channels
        instance.enable_channels(
            stored.device_number, stored.channel_mode, lime::TRXDir::Tx);

        // 6. Disable PA path
        toggle_pa_path(stored.device_number, false);
    }
}

sink_impl::~sink_impl()
{
    auto& instance = device_handler::getInstance();

    instance.get_device(stored.device_number)->StreamStop(0);
    instance.close_device(stored.device_number, sink_block);
}

bool sink_impl::start(void)
{
    auto& instance = device_handler::getInstance();

    std::unique_lock<std::recursive_mutex> lock(instance.block_mutex);
    // Init timestamp
    tx_meta.timestamp = 0;

    if (stream_analyzer) {
        t1 = std::chrono::high_resolution_clock::now();
        t2 = t1;
    }
    // Enable PA path
    toggle_pa_path(stored.device_number, true);

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
    return true;
}

bool sink_impl::stop(void)
{
    auto& instance = device_handler::getInstance();

    std::unique_lock<std::recursive_mutex> lock(instance.block_mutex);

    instance.get_device(stored.device_number)->StreamStop(0);

    // Disable PA path
    toggle_pa_path(stored.device_number, false);
    std::unique_lock<std::recursive_mutex> unlock(instance.block_mutex);
    return true;
}

int sink_impl::work(int noutput_items,
                    gr_vector_const_void_star& input_items,
                    gr_vector_void_star& output_items)
{
    // Init number of items to be sent and timestamps
    nitems_send = noutput_items;
    tx_meta.waitForTimestamp = false;
    tx_meta.flushPartialPacket = false;
    // Check if channel 0 has any tags
    work_tags(noutput_items);
    // If length tag has been found burst_length should be higher than 0
    if (burst_length > 0) {
        nitems_send = std::min<long>(burst_length, nitems_send);
        // Make sure to wait for timestamp
        tx_meta.waitForTimestamp = true;
        // Check if it is the end of the burst
        if (burst_length - static_cast<long>(nitems_send) == 0) {
            tx_meta.flushPartialPacket = true;
        }
    }

    // Print stream stats to debug
    if (stream_analyzer == true) {
        print_stream_stats(0);
    }

    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(stored.device_number);

    switch (instance.get_stream_config(stored.device_number).format) {
    case lime::DataFormat::F32:
        ret = device->StreamTx(
            0,
            reinterpret_cast<const lime::complex32f_t* const*>(input_items.data()),
            nitems_send,
            &tx_meta);
        break;
    case lime::DataFormat::I16:
        ret = device->StreamTx(
            0,
            reinterpret_cast<const lime::complex16_t* const*>(input_items.data()),
            nitems_send,
            &tx_meta);
        break;
    case lime::DataFormat::I12:
        ret = device->StreamTx(
            0,
            reinterpret_cast<const lime::complex12_t* const*>(input_items.data()),
            nitems_send,
            &tx_meta);
        break;

    default:
        throw std::logic_error("Unsupported format");
        break;
    }

    // Send data
    if (ret < 0) {
        return 0;
    }

    burst_length -= ret;
    tx_meta.timestamp += ret;
    consume_each(ret);

    return 0;
}

void sink_impl::work_tags(int noutput_items)
{
    std::vector<tag_t> tags;
    uint64_t current_sample = nitems_read(0);
    get_tags_in_range(tags, 0, current_sample, current_sample + noutput_items);

    if (tags.empty()) {
        return;
    }

    std::sort(tags.begin(), tags.end(), tag_t::offset_compare);
    // Go through the tags
    for (tag_t cTag : tags) {
        // Found tx_time tag
        if (pmt::eq(cTag.key, TIME_TAG)) {
            // Convert time to sample timestamp
            uint64_t secs = pmt::to_uint64(pmt::tuple_ref(cTag.value, 0));
            double fracs = pmt::to_double(pmt::tuple_ref(cTag.value, 1));
            uint64_t u_rate = static_cast<uint64_t>(stored.samp_rate);
            double f_rate = stored.samp_rate - u_rate;
            uint64_t timestamp =
                u_rate * secs + llround(secs * f_rate + fracs * stored.samp_rate);

            if (cTag.offset == current_sample) {
                tx_meta.waitForTimestamp = true;
                tx_meta.timestamp = timestamp;
            } else {
                nitems_send = static_cast<int>(cTag.offset - current_sample);
                break;
            }
        }
        // Found length tag
        else if (!pmt::is_null(LENGTH_TAG) && pmt::eq(cTag.key, LENGTH_TAG)) {
            if (cTag.offset == current_sample) {
                // Found length tag in the middle of the burst
                if (burst_length > 0 && ret > 0)
                    GR_LOG_WARN(d_logger, "Length tag has been preempted");
                burst_length = pmt::to_long(cTag.value);
            } else {
                nitems_send = static_cast<int>(cTag.offset - current_sample);
                break;
            }
        }
    }
}

// Print stream status
void sink_impl::print_stream_stats(int channel)
{
    t2 = std::chrono::high_resolution_clock::now();
    auto timePeriod =
        std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    if (timePeriod < 1000) {
        return;
    }

    lime::StreamStats status;
    device_handler::getInstance()
        .get_device(stored.device_number)
        ->StreamStatus(0, nullptr, &status);
    GR_LOG_INFO(d_logger,
                "---------------------------------------------------------------");
    GR_LOG_INFO(d_logger,
                fmt::format("TX |rate: {:f} MB/s |dropped packets: {:d} |FIFO: {:d}%",
                            status.dataRate_Bps / 1e6,
                            status.loss,
                            100 * status.FIFO.ratio()));
    GR_LOG_INFO(d_logger,
                "---------------------------------------------------------------");
    t1 = t2;
}

// Setup stream
void sink_impl::init_stream(int device_number, int channel)
{
    auto& instance = device_handler::getInstance();

    auto& config = instance.get_stream_config(device_number);
    config.channels.at(lime::TRXDir::Tx).push_back(channel);
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
        fmt::format("init_stream: sink channel {:d} (device nr. {:d}) stream setup done.",
                    channel,
                    device_number));
}

// Return io_signature to manage module input count
// based on SISO (one input) and MIMO (two inputs) modes
inline gr::io_signature::sptr sink_impl::args_to_io_signature(int channel_number)
{
    if (channel_number < 2) {
        return gr::io_signature::make(1, 1, sizeof(gr_complex));
    } else if (channel_number == 2) {
        return gr::io_signature::make(2, 2, sizeof(gr_complex));
    } else {
        throw std::invalid_argument(
            "sink_impl::args_to_io_signature(): channel_number must be 0, 1 or 2.");
    }
}

double sink_impl::set_center_freq(double freq, size_t chan)
{
    return device_handler::getInstance().set_rf_freq(
        stored.device_number, lime::TRXDir::Tx, 0, freq);
}

void sink_impl::set_antenna(int antenna, int channel)
{
    pa_path[channel] = antenna;
    device_handler::getInstance().set_antenna(
        stored.device_number, channel, lime::TRXDir::Tx, antenna);
}

void sink_impl::toggle_pa_path(int device_number, bool enable)
{
    auto& instance = device_handler::getInstance();
    auto device = instance.get_device(device_number);

    suppress_limesuite_logging();
    if (stored.channel_mode < 2) {
        if (device->SetAntenna(0,
                               lime::TRXDir::Tx,
                               stored.channel_mode,
                               enable ? pa_path[stored.channel_mode] : 0) !=
            lime::OpStatus::Success) {
            instance.error(device_number);
        }
    } else {
        if (device->SetAntenna(0, lime::TRXDir::Tx, 0, enable ? pa_path[0] : 0) !=
            lime::OpStatus::Success) {
            instance.error(device_number);
        }

        if (device->SetAntenna(0, lime::TRXDir::Tx, 1, enable ? pa_path[1] : 0) !=
            lime::OpStatus::Success) {
            instance.error(device_number);
        }
    }
    set_limesuite_logger(); // Restore our logging
}

void sink_impl::set_nco(float nco_freq, int channel)
{
    device_handler::getInstance().set_nco(
        stored.device_number, lime::TRXDir::Tx, channel, nco_freq);
}

double sink_impl::set_bandwidth(double analog_bandw, int channel)
{
    return device_handler::getInstance().set_analog_filter(
        stored.device_number, lime::TRXDir::Tx, channel, analog_bandw);
}

void sink_impl::set_digital_filter(double digital_bandw, int channel)
{
    device_handler::getInstance().set_digital_filter(
        stored.device_number, lime::TRXDir::Tx, channel, digital_bandw);
}

unsigned sink_impl::set_gain(unsigned gain_dB, int channel)
{
    return device_handler::getInstance().set_gain(
        stored.device_number, lime::TRXDir::Tx, channel, gain_dB);
}

void sink_impl::calibrate(double bandw, int channel)
{
    // PA path needs to be enabled for calibration
    toggle_pa_path(stored.device_number, true);
    device_handler::getInstance().calibrate(
        stored.device_number, lime::TRXDir::Tx, channel, bandw);
    toggle_pa_path(stored.device_number, false);
}

double sink_impl::set_sample_rate(double rate)
{
    device_handler::getInstance().set_samp_rate(stored.device_number, rate);
    stored.samp_rate = rate;
    return rate;
}

void sink_impl::set_buffer_size(uint32_t size) { stored.FIFO_size = size; }

void sink_impl::set_oversampling(int oversample)
{
    device_handler::getInstance().set_oversampling(stored.device_number, oversample);
}

void sink_impl::set_tcxo_dac(uint16_t dacVal)
{
    device_handler::getInstance().set_tcxo_dac(stored.device_number, dacVal);
}

void sink_impl::write_lms_reg(uint32_t address, uint16_t val)
{
    device_handler::getInstance().write_lms_reg(stored.device_number, address, val);
}

void sink_impl::set_gpio_dir(uint8_t dir)
{
    device_handler::getInstance().set_gpio_dir(stored.device_number, dir);
}

void sink_impl::write_gpio(uint8_t out)
{
    device_handler::getInstance().write_gpio(stored.device_number, out);
}

uint8_t sink_impl::read_gpio()
{
    return device_handler::getInstance().read_gpio(stored.device_number);
}

} // namespace limesdr
} // namespace gr
