/*
    Signals.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include <boost/signals2.hpp>
#include "ElementApp.h"

namespace Element {

using namespace boost::signals2;
using Signal       = signal<void()>;
using FloatSignal  = signal<void(float)>;
using StringSignal = signal<void(const String&)>;

}
