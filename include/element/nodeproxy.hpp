// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/element.hpp>

namespace element {

class NodeProxy {
public:
    NodeProxy() = delete;
    virtual ~NodeProxy() = default;

private:
    EL_DISABLE_COPY (NodeProxy)
};

} // namespace element
