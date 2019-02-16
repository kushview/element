#pragma once

#include "gui/nodes/NodeEditorComponent.h"
#include "engine/nodes/ProgramChangeMapNode.h"

namespace Element {

class ProgramChangeMapEditor : public NodeEditorComponent
{
public:
    ProgramChangeMapEditor (const Node& node);
    virtual ~ProgramChangeMapEditor();

    void paint (Graphics&) override;
    void resized() override;

    void addProgram();
    void removeSelectedProgram();
    int getNumPrograms() const;
    ProgramChangeMapNode::ProgramEntry getProgram (int) const;
    void setProgram (int, ProgramChangeMapNode::ProgramEntry);
    void sendProgram (int);

    void selectRow (int row);
    void setStoreSize (const bool storeSize);

private:
    Node node;
    class TableModel; friend class TableModel;
    std::unique_ptr<TableModel> model;
    TableListBox table;
    TextButton addButton;
    TextButton delButton;
    bool storeSizeInNode = true;
    SignalConnection lastProgramChangeConnection;
    void selectLastProgram();
};

}
