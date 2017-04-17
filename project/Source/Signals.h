/*
    Signals.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#ifndef EL_SIGNALS_H
#define EL_SIGNALS_H

#include <boost/bind.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist_hook.hpp>
#include <boost/signals2/signal.hpp>
#include "ElementApp.h"

namespace Element {

typedef boost::signals2::signal<void()> Signal;
typedef boost::signals2::signal<void(float)> FloatSignal;
typedef boost::signals2::signal<void(const String&)> StringSignal;

}

#endif

