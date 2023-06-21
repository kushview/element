// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/nodefactory.hpp>
#include <element/nodeobject.hpp>

namespace element {

class LV2Node : public NodeObject {
public:
};

class LV2NodeProvider : public NodeProvider {
public:
    LV2NodeProvider();
    ~LV2NodeProvider();
    String format() const override { return "LV2"; }
    NodeObject* create (const String&) override;
    StringArray findTypes() override;

private:
    class LV2;
    std::unique_ptr<LV2> lv2;
};

} // namespace element
