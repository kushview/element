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

#include "ElementApp.h"
#include <element/node.hpp>

namespace element {

class GraphNode;
class ContentComponent;
class GraphEditorComponent;

class NodeAudioBusesComponent : public AudioProcessorEditor,
                                public Button::Listener
{
public:
    class InputOutputConfig;

    NodeAudioBusesComponent (const Node& n, AudioProcessor* const p, ContentComponent* cc = nullptr);
    ~NodeAudioBusesComponent();

    void paint (Graphics& g) override;
    void resized() override;

    InputOutputConfig* getConfig (bool isInput) noexcept { return isInput ? inConfig : outConfig; }
    void update();

    void buttonClicked (Button*) override;

private:
    ContentComponent* content = nullptr;
    ContentComponent* getContentComponent();
    GraphEditorComponent* getGraphEditor() const;
    GraphNode* getGraph() const;
    int32 getNodeId() const;

    friend class InputOutputConfig;
    Node node;

    AudioProcessor::BusesLayout currentLayout;

    Label title;
    ScopedPointer<InputOutputConfig> inConfig, outConfig;
    TextButton saveButton, cancelButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeAudioBusesComponent)
};
} // namespace element
