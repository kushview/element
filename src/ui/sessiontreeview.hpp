// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/content.hpp>

namespace element {

class SessionTreePanel;

class SessionTreeContentView : public ContentView
{
public:
    SessionTreeContentView();
    ~SessionTreeContentView();

    void didBecomeActive() override;
    void stabilizeContent() override;
    void resized() override;

private:
    std::unique_ptr<SessionTreePanel> tree;
};

} // namespace element
