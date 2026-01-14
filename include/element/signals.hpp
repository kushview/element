// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/signals2.hpp>

namespace element {
template <typename T>
using Signal = boost::signals2::signal<T>;
using SignalConnection = boost::signals2::connection;
using SharedConnectionBlock = boost::signals2::shared_connection_block;
} // namespace element
