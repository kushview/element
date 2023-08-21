// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/nodeeditor.hpp>
#include "matrixstate.hpp"

namespace element {

class AudioRouterEditor : public NodeEditor,
                          public ChangeListener
{
public:
    AudioRouterEditor (const Node& node);
    ~AudioRouterEditor();

    void resized() override;
    void paint (Graphics& g) override;

    void adjustBoundsToMatrixSize (int cellSize = 0);
    String getSizeString() const;
    MatrixState& getMatrixState() { return matrix; }
    void applyMatrix();
    void setFadeLength (double length);
    void changeListenerCallback (ChangeBroadcaster*) override;

    void setAutoResize (bool yn) { autoResize = yn; }

private:
    MatrixState matrix;
    bool autoResize = false;
    class Content;
    std::unique_ptr<Content> content;
};

} // namespace element
