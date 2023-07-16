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

#include "lv2/workerfeature.hpp"

namespace element {

namespace LV2Callbacks {
static LV2_Worker_Status scheduleWork (LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
{
    WorkerFeature* worker = reinterpret_cast<WorkerFeature*> (handle);
    if (! worker->scheduleWork (size, data))
        return LV2_WORKER_ERR_NO_SPACE;
    return LV2_WORKER_SUCCESS;
}

static LV2_Worker_Status workRespond (LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
{
    WorkerFeature* worker = reinterpret_cast<WorkerFeature*> (handle);
    if (! worker->respondToWork (size, data))
        return LV2_WORKER_ERR_NO_SPACE;
    return LV2_WORKER_SUCCESS;
}
} // namespace LV2Callbacks

WorkerFeature::WorkerFeature (WorkThread& thread, uint32_t bufsize, LV2_Handle handle, LV2_Worker_Interface* iface)
    : WorkerBase (thread, bufsize)
{
    setInterface (handle, iface);

    uri = LV2_WORKER__schedule;
    feat.URI = uri.toRawUTF8();
    data.handle = this;
    data.schedule_work = &LV2Callbacks::scheduleWork;
    feat.data = (void*) &data;
}

WorkerFeature::~WorkerFeature()
{
    plugin = nullptr;
    worker = nullptr;
    feat = {};
    data = {};
    uri = {};
}

void WorkerFeature::setInterface (LV2_Handle handle, LV2_Worker_Interface* iface)
{
    plugin = handle;
    worker = iface;
}

const String& WorkerFeature::getURI() const { return uri; }
const LV2_Feature* WorkerFeature::getFeature() const { return &feat; }

void WorkerFeature::processRequest (uint32_t size, const void* requestData)
{
    jassert (worker != nullptr && plugin != nullptr);
    worker->work (plugin, LV2Callbacks::workRespond, this, size, requestData);
}

void WorkerFeature::processResponse (uint32_t size, const void* responseData)
{
    jassert (worker != nullptr && worker->work_response != nullptr && plugin != nullptr);
    worker->work_response (plugin, size, responseData);
}

void WorkerFeature::endRun()
{
    jassert (worker != nullptr && plugin != nullptr);
    if (worker->end_run)
        worker->end_run (plugin);
}

} // namespace element
