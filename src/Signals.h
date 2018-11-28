/*
    Signals.h - This file is part of Element
    Copyright (C) 2014-2018  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include <boost/signals2.hpp>

namespace Element
{
    template<typename T> using Signal = boost::signals2::signal<T>;
    using SharedConnectionBlock = boost::signals2::shared_connection_block;
}
