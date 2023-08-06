// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/content.hpp>

#define EL_VIEW_CONTROLLERS "ControllersView"

namespace element {

class ControllersView : public ContentView
{
public:
    ControllersView();
    virtual ~ControllersView();

    void resized() override;
    void stabilizeContent() override;
    void initializeView (Services&) override;

private:
    class Content;
    std::unique_ptr<Content> content;
};

} // namespace element
