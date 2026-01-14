// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include <lvtk/symbols.hpp>

namespace element {

class SymbolMap final {
public:
    SymbolMap() noexcept {}
    ~SymbolMap() {}

    const uint32_t map (const char* str) noexcept { return _sym.map (str); }
    const char* unmap (uint32_t urid) noexcept { return _sym.unmap (urid); }

    inline auto mapPtr() const noexcept { return (LV2_URID_Map*) _sym.map_feature()->data; }
    inline auto mapFeature() const noexcept { return _sym.map_feature(); }
    inline auto unmapPtr() const noexcept { return (LV2_URID_Unmap*) _sym.unmap_feature()->data; }
    inline auto unmapFeature() const noexcept { return _sym.unmap_feature(); }

    inline operator LV2_URID_Map*() const noexcept { return mapPtr(); }

private:
    lvtk::Symbols _sym;
};

} // namespace element
