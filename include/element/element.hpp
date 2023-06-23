// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once
#include <element/element.h>

#define EL_DISABLE_COPY(ClassName)         \
    ClassName (const ClassName&) = delete; \
    ClassName& operator= (const ClassName&) = delete;
#define EL_DISABLE_MOVE(ClassName)          \
    ClassName (const ClassName&&) = delete; \
    ClassName& operator= (const ClassName&&) = delete;

namespace element {

/** Ignore a set of variables... */
template <typename... Args>
inline void ignore (Args&&...) noexcept
{
}

} // namespace element
