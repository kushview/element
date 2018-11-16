/*
    Signals.h - This file is part of Element
    Copyright (C) 2014-2018  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "JuceHeader.h"
#include <boost/signals2.hpp>

namespace Element {

using Signal       = boost::signals2::signal<void()>;
using FloatSignal  = boost::signals2::signal<void(float)>;

}
