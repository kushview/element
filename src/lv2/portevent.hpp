/*
    Copyright (c) 2014-2019  Michael Fisher <mfisher@kushview.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

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
