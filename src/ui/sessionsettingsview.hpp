// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ui/buttons.hpp"
#include <element/ui/content.hpp>

#define EL_VIEW_SESSION_SETTINGS "SessionSettingsView"

namespace element {
class SessionPropertyPanel;
class SessionContentView : public ContentView
{
public:
    SessionContentView();
    ~SessionContentView();

    void resized() override;
    void didBecomeActive() override;
    void paint (Graphics& g) override;

private:
    std::unique_ptr<SessionPropertyPanel> props;
};

typedef SessionContentView SessionSettingsView;
} // namespace element
