// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "sol/forward.hpp"

namespace element {

class Context;

namespace Lua {
extern void initializeState (sol::state_view&);
extern void initializeState (sol::state_view&, Context&);
extern void setGlobals (sol::state_view&, Context&);
extern void clearGlobals (sol::state_view&);
} // namespace Lua

} // namespace element
