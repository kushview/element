// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace element {

/** A scoped function callback. Similar to goto, this calls a function when it goes out of scope.
 
    You should not perform complex operations with this.  It is meant to ensure a callback  when
    the calling function returns.
 */
class ScopedCallback
{
public:
    /** Create a new ScopedCallback.
       @param f    The function to call
    */
    explicit ScopedCallback (std::function<void()> f)
        : callback (f) {}

    ~ScopedCallback()
    {
        if (callback)
            callback();
    }

private:
    std::function<void()> callback;
};

} // namespace element
