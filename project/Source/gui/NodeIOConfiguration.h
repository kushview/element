
#pragma once

#include "ElementApp.h"
#include "session/Node.h"

namespace Element {

class GraphEditorComponent;
class GraphProcessor;
class MainWindow;

class NodeAudioBusesComponent : public  AudioProcessorEditor
{
public:
    class InputOutputConfig;

    NodeAudioBusesComponent (const Node& n, AudioProcessor* const p);
    ~NodeAudioBusesComponent();

    void paint (Graphics& g) override;
    void resized() override;

    InputOutputConfig* getConfig (bool isInput) noexcept { return isInput ? inConfig : outConfig; }
    void update();

private:
    MainWindow* getMainWindow() const;
    GraphEditorComponent* getGraphEditor() const;
    GraphProcessor* getGraph() const;
    int32 getNodeId() const;

    friend class InputOutputConfig;
    Node node;
    
    AudioProcessor::BusesLayout currentLayout;
    Label title;
    ScopedPointer<InputOutputConfig> inConfig, outConfig;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeAudioBusesComponent)
};
}
