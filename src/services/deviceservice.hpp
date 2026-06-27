// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/services.hpp>

namespace element {

class DeviceService : public Service
{
public:
    DeviceService();
    ~DeviceService();

    void activate() override;
    void deactivate() override;

    /** Rebuild live MIDI mapping bindings from the current session. */
    void refresh();

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace element
