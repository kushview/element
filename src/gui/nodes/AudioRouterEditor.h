#pragma once

#include "gui/nodes/NodeEditorComponent.h"
#include "engine/nodes/MidiProgramMapNode.h"

namespace Element {

class AudioRouterEditor : public NodeEditorComponent,
                          public ChangeListener
{
public:
    AudioRouterEditor (const Node& node);
    ~AudioRouterEditor();

    void resized() override;
    void paint (Graphics& g) override;

    MatrixState& getMatrixState() { return matrix; }
    void applyMatrix();
    void setFadeLength (double length);
    void changeListenerCallback (ChangeBroadcaster*) override;

private:
    MatrixState matrix;
    class Content;
    std::unique_ptr<Content> content;
};

}
