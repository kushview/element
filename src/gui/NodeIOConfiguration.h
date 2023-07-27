// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ElementApp.h"
#include <element/node.hpp>

namespace element {

class GraphNode;
class Content;
class GraphEditorComponent;

class NodeAudioBusesComponent : public AudioProcessorEditor,
                                public Button::Listener
{
public:
    class InputOutputConfig;

    NodeAudioBusesComponent (const Node& n, AudioProcessor* const p, Content* cc = nullptr);
    ~NodeAudioBusesComponent();

    void paint (Graphics& g) override;
    void resized() override;

    InputOutputConfig* getConfig (bool isInput) noexcept { return isInput ? inConfig.get() : outConfig.get(); }
    void update();

    void buttonClicked (Button*) override;

private:
    Content* _content = nullptr;
    Content* content();
    GraphEditorComponent* getGraphEditor() const;
    GraphNode* getGraph() const;
    int32 getNodeId() const;

    friend class InputOutputConfig;
    Node node;

    AudioProcessor::BusesLayout currentLayout;

    Label title;
    std::unique_ptr<InputOutputConfig> inConfig, outConfig;
    TextButton saveButton, cancelButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeAudioBusesComponent)
};
} // namespace element
