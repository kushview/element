
#pragma once

#include "gui/ContentComponent.h"
#include "gui/widgets/MidiChannelSelectComponent.h"

namespace Element {

class NodeUIContentView : public ContentView
{
public:
    NodeUIContentView();
    ~NodeUIContentView();

    void stabilizeContent() override;
    void resized() override;
    void paint (Graphics& g) override;

private:
    Node node;
    SignalConnection selectedNodeConnection;
    std::unique_ptr<Component> editor;
    void clearEditor();
    Component* createEmbededEditor();
};

}
