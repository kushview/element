/*
    This file is part of Element
    Copyright (C) 2014-2021  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <boost/signals2.hpp>

namespace Element {
template <typename T>
using Signal = boost::signals2::signal<T>;
using SignalConnection = boost::signals2::connection;
using SharedConnectionBlock = boost::signals2::shared_connection_block;
} // namespace Element
