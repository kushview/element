// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/guicommon.hpp"
#include <element/node.hpp>

namespace element {

class BreadCrumbComponent : public Component
{
public:
    BreadCrumbComponent() { setSize (300, 24); }
    ~BreadCrumbComponent() {}

    inline void resized() override
    {
        auto r = getLocalBounds();
        for (int i = 0; i < segments.size(); ++i)
        {
            segments[i]->setBounds (r.removeFromLeft (segments[i]->getWidth()));
            if (auto* div = dividers[i])
            {
                div->setBounds (r.removeFromLeft (div->getWidth()));
            }
        }
    }

    inline void setNode (const Node& newNode)
    {
        nodes.clear();
        segments.clearQuick (true);
        dividers.clearQuick (true);
        nodes.insert (0, newNode);
        Node nextNode = newNode.getParentGraph();
        while (nextNode.isValid())
        {
            nodes.insert (0, nextNode);
            nextNode = nextNode.getParentGraph();
        }

        int i = 0;
        for (auto& node : nodes)
        {
            auto* seg = segments.add (new Label());
            seg->getTextValue().referTo (node.getPropertyAsValue (tags::name));
            GlyphArrangement glyphs;
            glyphs.addLineOfText (seg->getFont(), node.getName(), 0, 0);
            seg->setSize (2 + (int) glyphs.getBoundingBox (0, -1, true).getWidth(), getHeight());
            seg->setJustificationType (Justification::centred);
            addAndMakeVisible (seg);
            if (++i != nodes.size())
            {
                auto* div = dividers.add (new Label());
                div->setText ("/", dontSendNotification);
                GlyphArrangement divGlyphs;
                divGlyphs.addLineOfText (seg->getFont(), "/", 0, 0);
                div->setSize (10 + (int) divGlyphs.getBoundingBox (0, -1, true).getWidth(), getHeight());

                div->setJustificationType (Justification::centred);
                addAndMakeVisible (div);
            }
        }

        resized();
    }

private:
    Array<Node> nodes;
    OwnedArray<Label> segments;
    OwnedArray<Label> dividers;
};
} // namespace element
