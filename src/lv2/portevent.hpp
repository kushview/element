// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <cstdint>

namespace element {

/** A function which writes to a port
    @param port         Port Index
    @param size         Data size
    @param protocol     Data protocol
    @param data         The Data
*/
using PortWriteFunction = std::function<void (uint32_t port, uint32_t size, uint32_t protocol, void const* data)>;

/** Same as PortWriteFunction except is indended to trigger when
    events come from the plugin instance. */
using PortNotificationFunction = PortWriteFunction;

/** A simple type for writing/reading port values/messages through a ringbuffer */
struct PortEvent
{
    uint32_t index; ///< The port index
    uint32_t protocol; ///< The port protocol
    union
    {
        double decimal; ///< Timestamp as a decimal, units depends on context
        int64_t frames; ///< Timestamp in audio frames
    } time;
    uint32_t size; ///< The size of data
};

} // namespace element
