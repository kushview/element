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

private:
    class TableModel; friend class TableModel;
    std::unique_ptr<TableModel> model;
    TableListBox table;
};

}
