/* -*- c++ -*- */
/*
 * Copyright 2023 Lime Microsystems info@limemicro.com
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "logging.h"

#include <limesuiteng/Logger.h>

#include <gnuradio/logger.h>

#include <cassert>
#include <mutex>

static gr::logger_ptr logger;
static gr::logger_ptr debug_logger;

static std::mutex log_handler_mutex;

static void gr_loghandler(const lime::LogLevel level, const char* message)
{
    assert(logger);
    assert(message);

    switch (level) {
    case lime::LogLevel::Critical:
        GR_LOG_CRIT(logger, message);
        break;

    case lime::LogLevel::Error:
        GR_LOG_ERROR(logger, message);
        break;

    case lime::LogLevel::Warning:
        GR_LOG_WARN(logger, message);
        break;

    case lime::LogLevel::Info:
        GR_LOG_INFO(logger, message);
        break;

    case lime::LogLevel::Debug:
        GR_LOG_DEBUG(debug_logger, message);
        break;

    default:
        break;
    }
}

void set_limesuite_logger(void)
{
    std::lock_guard<std::mutex> lock(log_handler_mutex);
    if (!logger)
        gr::configure_default_loggers(logger, debug_logger, "Lime Suite NG");

    lime::registerLogHandler(gr_loghandler);
}

void suppress_limesuite_logging(void)
{
    std::lock_guard<std::mutex> lock(log_handler_mutex);

    lime::registerLogHandler([](const lime::LogLevel, const char*) {});
}
