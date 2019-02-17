
#pragma once

#include "gui/ContentComponent.h"
#include "gui/widgets/NodeListComboBox.h"
#include "gui/Buttons.h"

namespace Element {

class NodeEditorContentView : public ContentView,
                              public ComboBox::Listener
{
public:
    NodeEditorContentView();
    ~NodeEditorContentView();

    void setSticky (bool shouldBeSticky);
    bool isSticky() const { return sticky; }
    void stabilizeContent() override;
    void resized() override;
    void paint (Graphics& g) override;

    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
private:
    Node graph, node;
    SignalConnection selectedNodeConnection;
    std::unique_ptr<Component> editor;
    NodeListComboBox nodesCombo;
    IconButton menuButton;
    bool sticky = true;
    void clearEditor();
    Component* createEmbededEditor();
    void setNode (const Node&);
    static void nodeMenuCallback (int, NodeEditorContentView*);
};

}
