// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/nodefactory.hpp>

namespace element {

class SymbolMap;
class Processor;

//==============================================================================
class LV2NodeProvider : public NodeProvider {
public:
    LV2NodeProvider();
    LV2NodeProvider (SymbolMap&);
    ~LV2NodeProvider();
    juce::String format() const override { return "LV2"; }
    Processor* create (const juce::String&) override;
    FileSearchPath defaultSearchPath() override;
    juce::StringArray findTypes (const juce::FileSearchPath&, bool, bool) override;
    String nameForURI (const String& uri) const noexcept;
    void scan (const String& URI, OwnedArray<PluginDescription>& out) override;

private:
    class LV2;
    std::unique_ptr<LV2> lv2;
};

} // namespace element
