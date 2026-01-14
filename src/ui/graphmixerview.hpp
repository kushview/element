// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/content.hpp>

#define EL_VIEW_GRAPH_MIXER "GraphMixerView"

namespace element {

class GraphMixerView : public ContentView
{
public:
    GraphMixerView();
    ~GraphMixerView();

    void resized() override;
    void stabilizeContent() override;
    void initializeView (Services&) override;

private:
    class Content;
    friend class Content;
    std::unique_ptr<Content> content;
};

} // namespace element
