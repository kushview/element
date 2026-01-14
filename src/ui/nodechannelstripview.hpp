// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/content.hpp>

namespace element {

class NodeChannelStripView : public ContentView
{
public:
    NodeChannelStripView();
    ~NodeChannelStripView();

    void resized() override;
    void stabilizeContent() override;
    void initializeView (Services&) override;

private:
    class Content;
    friend class Content;
    std::unique_ptr<Content> content;
};

} // namespace element
