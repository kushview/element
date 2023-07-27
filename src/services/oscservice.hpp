// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/services.hpp>

namespace element {

class OSCService : public Service
{
public:
    OSCService();
    ~OSCService();

    /** Sets the port and starts/stops the host accoding to Settings
        
        @param alertOnFail  If true, show an alert if the host could not start
    */
    void refreshWithSettings (bool alertOnFail = false);

    void activate() override;
    void deactivate() override;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace element
