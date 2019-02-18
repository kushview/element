
#pragma once

#include "gui/nodes/NodeEditorComponent.h"

namespace Element {

class GenericNodeEditor : public NodeEditorComponent
{
public:
    GenericNodeEditor (const Node&);
    ~GenericNodeEditor() override;

    AudioProcessor* getAudioProcessor() const;

    void resized() override;
    void paint (Graphics&) override;
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

}
