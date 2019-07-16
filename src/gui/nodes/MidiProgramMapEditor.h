#pragma once

#include "gui/nodes/NodeEditorComponent.h"
#include "engine/nodes/MidiProgramMapNode.h"

namespace Element {

class MidiProgramMapEditor : public NodeEditorComponent,
                             public ChangeListener
{
public:
    MidiProgramMapEditor (const Node& node);
    virtual ~MidiProgramMapEditor();

    void paint (Graphics&) override;
    void resized() override;

    void addProgram();
    void removeSelectedProgram();
    int getNumPrograms() const;
    MidiProgramMapNode::ProgramEntry getProgram (int) const;
    void setProgram (int, MidiProgramMapNode::ProgramEntry);
    void sendProgram (int);

    float getFontSize() const { return fontSize; }
    void setFontSize (float newSize);

    void selectRow (int row);
    void setStoreSize (const bool storeSize);

    inline void changeListenerCallback (ChangeBroadcaster*) override { table.updateContent(); }
    
    bool keyPressed (const KeyPress&) override;
    
private:
    Node node;
    class TableModel; friend class TableModel;
    std::unique_ptr<TableModel> model;
    TableListBox table;
    TextButton addButton;
    TextButton delButton;
    bool storeSizeInNode = true;
    float fontSize = 15.f;
    SignalConnection lastProgramChangeConnection;
    void selectLastProgram();
};

}
