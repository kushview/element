#pragma once

#include "JuceHeader.h"

namespace Element {

class ProgramChangeMapEditor : public Component
{
public:
    ProgramChangeMapEditor();
    ~ProgramChangeMapEditor();

    void paint (Graphics&) override;
    void resized() override;

    void addProgram();
    void removeSelectedProgram();
    
private:
    class TableModel; friend class TableModel;
    std::unique_ptr<TableModel> model;
    TableListBox table;
    TextButton addButton;
    TextButton delButton;
};

}
