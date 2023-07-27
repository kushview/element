// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ElementApp.h"
#include <element/ui/content.hpp>
#include "gui/PatchMatrixComponent.h"

namespace element {

class BreadCrumbComponent;
class Node;

class ConnectionGrid : public ContentView,
                       public DragAndDropTarget
{
public:
    ConnectionGrid();
    ~ConnectionGrid();

    void setNode (const Node& node);

    void didBecomeActive() override;

    void paint (Graphics&) override;
    void resized() override;

    void mouseDown (const MouseEvent& ev) override;

    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;

#if 0
    void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& dragSourceDetails) override;
    bool shouldDrawDragImageWhenOver() override;
#endif

private:
    std::unique_ptr<BreadCrumbComponent> breadcrumb;

    friend class PatchMatrix;
    class PatchMatrix;
    PatchMatrix* matrix;

    friend class Controls;
    class Controls;
    Controls* controls;

    friend class Sources;
    class Sources;
    Sources* sources;

    friend class Destinations;
    class Destinations;
    Destinations* destinations;

    friend class Quads;
    class Quads;
    std::unique_ptr<QuadrantLayout> quads;
};

} // namespace element
