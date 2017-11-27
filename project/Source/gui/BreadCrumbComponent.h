
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
                seg->setSize (4 + seg->getFont().getStringWidth (node.getName()), getHeight());
                addAndMakeVisible (seg);
                if (++i != nodes.size())
                {
                    auto* div = dividers.add (new Label ());
                    div->setText ("/", dontSendNotification);
                    div->setSize (4 + div->getFont().getStringWidth ("/"), getHeight());
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
