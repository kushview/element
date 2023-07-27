// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <lvtk/ext/worker.hpp>

#include "lv2/lv2features.hpp"
#include "lv2/workthread.hpp"

namespace element {

class WorkerFeature final : public LV2Feature,
                            public WorkerBase
{
public:
    WorkerFeature (WorkThread& thread, uint32_t bufsize, LV2_Handle handle = nullptr, LV2_Worker_Interface* iface = nullptr);

    ~WorkerFeature();

    void setInterface (LV2_Handle handle, LV2_Worker_Interface* iface);

    const String& getURI() const;
    const LV2_Feature* getFeature() const;

    void endRun();
    void processRequest (uint32_t size, const void* data);
    void processResponse (uint32_t size, const void* data);

private:
    juce::String uri;
    LV2_Worker_Interface* worker;
    LV2_Handle plugin;
    LV2_Worker_Schedule data;
    LV2_Feature feat;
};

} // namespace element
