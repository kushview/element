// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "./native.hpp"

// #define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
// #define JUCE_CORE_INCLUDE_OBJC_HELPERS 1
#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
// #define JUCE_GUI_BASICS_INCLUDE_SCOPED_THREAD_DPI_AWARENESS_SETTER 1
// #define JUCE_GRAPHICS_INCLUDE_COREGRAPHICS_HELPERS 1
#include <element/juce/gui_basics.hpp>

using namespace juce;

namespace element {

#if JUCE_LINUX
inline static Display* display()
{
    return XWindowSystem::getInstance()->getDisplay();
}
void reparentWindow (intptr_t child, intptr_t parent, int x, int y)
{
    XWindowSystemUtilities::ScopedXLock xLock;
    // X11Symbols::getInstance()->xUnmapWindow (display(), child);
    X11Symbols::getInstance()->xReparentWindow (display(),
                                                (::Window) child,
                                                (::Window) parent,
                                                x,
                                                y);
}
#else
void reparentWindow (intptr_t child, intptr_t parent, int x, int y)
{
    juce::ignoreUnused (child, parent, x, y);
}
#endif

} // namespace element
