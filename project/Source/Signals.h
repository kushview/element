/*
    Signals.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

