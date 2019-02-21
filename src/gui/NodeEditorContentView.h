
#pragma once

#include "gui/ContentComponent.h"
#include "gui/widgets/NodeListComboBox.h"
#include "gui/Buttons.h"

namespace Element {

class NodeEditorContentView : public ContentView,
                              public ComboBox::Listener,
                              public Value::Listener
{
public:
    NodeEditorContentView();
    ~NodeEditorContentView();

    void setSticky (bool shouldBeSticky);
    bool isSticky() const { return sticky; }
    
    Node getNode()  const { return node; }
    void setNode (const Node&);
    
    void stabilizeContent() override;
    void resized() override;
    void paint (Graphics& g) override;

    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void valueChanged (Value& value) override;

private:
    Node graph, node;
    Value nodeObjectValue;
    SignalConnection selectedNodeConnection;
    std::unique_ptr<Component> editor;
    NodeListComboBox nodesCombo;
    IconButton menuButton;
    bool sticky = true;
    void clearEditor();
    Component* createEmbededEditor();
    
    static void nodeMenuCallback (int, NodeEditorContentView*);

    class NodeWatcher; std::unique_ptr<NodeWatcher> watcher;
};

}
