/*
    Copyright (c) 2014-2019  Michael Fisher <mfisher@kushview.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

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
