
#pragma once

#include "gui/ContentComponent.h"
#include "gui/widgets/MidiChannelSelectComponent.h"
#include "gui/widgets/NodeMidiProgramComponent.h"

namespace Element {

class NodeMidiContentView : public ContentView
{
public:
    NodeMidiContentView();
    ~NodeMidiContentView();

    void stabilizeContent() override;

    void resized() override;
    void paint (Graphics& g) override;

private:
    Node node;
    SignalConnection selectedNodeConnection;
    SignalConnection midiProgramChangedConnection;

    class SignalLabel : public Label
    {
    public:
        SignalLabel() { }
        ~SignalLabel() { }
        
        inline void mouseDoubleClick (const MouseEvent& ev) override
        { 
            if (onDoubleClicked)
                onDoubleClicked (ev);
        }

        std::function<void(const MouseEvent&)> onDoubleClicked;
    };

    PropertyPanel props;
    NodeObjectSync nodeSync;
    void updateProperties();
    void updateMidiProgram();
};

}
