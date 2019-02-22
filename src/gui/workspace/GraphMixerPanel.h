#pragma once

#include "gui/views/GraphMixerView.h"
#include "gui/workspace/ContentViewPanel.h"

namespace Element {

class GraphMixerPanel : public ContentViewPanel<GraphMixerView>
{
public:
    GraphMixerPanel() { setName ("Graph Mixer"); }
    ~GraphMixerPanel() = default;
};

}
