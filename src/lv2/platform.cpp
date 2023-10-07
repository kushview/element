// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#if defined(_MSC_VER)
#include <windows.h>
#include <winuser.h>
#elif defined(__linux__)
// #define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
// #define JUCE_CORE_INCLUDE_OBJC_HELPERS 1
#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
// #define JUCE_GUI_BASICS_INCLUDE_SCOPED_THREAD_DPI_AWARENESS_SETTER 1
// #define JUCE_GRAPHICS_INCLUDE_COREGRAPHICS_HELPERS 1
#include <element/juce/gui_basics.hpp>
#endif

#include <cmath>
#include <iostream>

namespace element {

#if defined(_MSC_VER)
bool getNativeWinodwSize (void* window, int& width, int& height)
{
    HWND w = (HWND) window;
    RECT r;
    if (GetWindowRect (w, &r))
    {
        width = std::abs (r.right - r.left);
        height = std::abs (r.bottom - r.top);
        return true;
    }

    return false;
}
#elif defined(__linux__)
inline static Display* xDisplay()
{
    return juce::XWindowSystem::getInstance()->getDisplay();
}

bool getNativeWinodwSize (void* win, int& width, int& height)
{
    ::Window window = (::Window) win;

    juce::XWindowSystemUtilities::ScopedXLock xLock;
    auto X11 = juce::X11Symbols::getInstance();
    XWindowAttributes atts;
    const auto res = True == X11->xGetWindowAttributes (xDisplay(), window, &atts);

    if (res)
    {
        width = static_cast<int> (atts.width);
        height = static_cast<int> (atts.height);
    }

    return res;
}
#endif
} // namespace element
