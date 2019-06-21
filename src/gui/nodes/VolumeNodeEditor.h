#pragma once

#include "gui/nodes/NodeEditorComponent.h"

namespace Element {

class Node;
class GuiController;

class VolumeNodeEditor : public NodeEditorComponent
{
public:
    VolumeNodeEditor (const Node&, GuiController&);
    ~VolumeNodeEditor();

    void paint (Graphics& g) override;
    void resized() override;
private:
    class ChannelStrip; friend class ChannelStrip;
    std::unique_ptr<ChannelStrip> strip;
};

}
