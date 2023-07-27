// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/arc.hpp>

namespace element {

Arc::Arc (uint32_t sn, uint32_t sp, uint32_t dn, uint32_t dp) noexcept
    : sourceNode (sn), sourcePort (sp), destNode (dn), destPort (dp)
{
}

} // namespace element
