/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "gui/GuiCommon.h"
#include "session/Node.h"

namespace Element {
    
    class BreadCrumbComponent : public Component
    {
    public:
        BreadCrumbComponent() { setSize (300, 24); }
        ~BreadCrumbComponent() { }
        
        inline void resized() override
        {
            auto r = getLocalBounds();
            for (int i = 0; i < segments.size(); ++i)
            {
                segments[i]->setBounds (r.removeFromLeft (segments[i]->getWidth()));
                if (auto* div = dividers [i]) {
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
                auto* seg = segments.add (new Label ());
                seg->getTextValue().referTo (node.getPropertyAsValue (Tags::name));
                seg->setSize (2 + seg->getFont().getStringWidth (node.getName()), getHeight());
                seg->setJustificationType (Justification::centred);
                addAndMakeVisible (seg);
                if (++i != nodes.size())
                {
                    auto* div = dividers.add (new Label ());
                    div->setText ("/", dontSendNotification);
                    div->setSize (10 + seg->getFont().getStringWidth ("/"), getHeight());

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
}
