// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/services.hpp>
#include <element/controller.hpp>

namespace element {

class DeviceService : public Service
{
public:
    DeviceService();
    ~DeviceService();

    void activate() override;
    void deactivate() override;

    void add (const Controller&);
    void add (const Controller&, const Control&);
    void add (const File& file);
    void remove (const Controller&);
    void remove (const Controller&, const Control&);
    void refresh (const Controller&);
    void refresh();

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace element
