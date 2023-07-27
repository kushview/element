// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <lvtk/ext/log.hpp>

#include <element/juce/core.hpp>

#include "lv2/lv2features.hpp"

namespace element {

class LogFeature : public LV2Feature
{
public:
    LogFeature();
    ~LogFeature();

    inline const juce::String& getURI() const { return uri; }
    inline const LV2_Feature* getFeature() const { return &feat; }

private:
    juce::String uri;
    LV2_Feature feat;
    LV2_Log_Log log;
};

} // namespace element
