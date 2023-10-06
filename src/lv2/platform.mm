// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#import <AppKit/AppKit.h>

#include <iostream>

namespace element {

bool getNativeWinodwSize (void* v, int& width, int& height)
{
    NSView* view = (NSView*) v;
    width = static_cast<int> (view.frame.size.width);
    height = static_cast<int> (view.frame.size.height);
    return true;
}

}
