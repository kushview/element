// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

namespace element {

/** The runtime mode of Element */
enum struct RunMode : int {
    /** Running as standalone application */
    Standalone = 0,
    /** Running as an audio plugin */
    Plugin = 1
};

} // namespace element
