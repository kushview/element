/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "ElementApp.h"
#include "engine/GraphProcessor.h"
#include "gui/ViewHelpers.h"

namespace Element {

class BlockComponent;
class BlockFactory;
class ConnectorComponent;
class PortComponent;
class PluginWindow;

/** A panel that displays and edits a GraphProcessor. */
class GraphEditorComponent   : public Component,
                               public ChangeListener,
                               public DragAndDropTarget,
                               private ValueTree::Listener,
                               public ViewHelperMixin
{
public:
    GraphEditorComponent();
    virtual ~GraphEditorComponent();

    /** Set the currently displayed graph */
    void setNode (const Node& n);

    /** Returns the displayed graph */
    Node getGraph() const { return graph; }
    
    /** Selects the given node */
    void selectNode (const Node& n);

    /** Removes the selected nodes */    
    void deleteSelectedNodes();
    
    /** Returns true if the layout is vertical */
    bool isLayoutVertical() const { return verticalLayout; }

    /** Changes the layout to vertical or not */
    void setVerticalLayout (const bool isVertical);

    /** Stabilize all nodes without changing position */
    void stabilizeNodes();

    void updateComponents();

    bool areResizePositionsFrozen() const { return resizePositionsFrozen; }
    inline void setResizePositionsFrozen (const bool shouldBeFrozen)
    {
        if (resizePositionsFrozen == shouldBeFrozen)
            return;
        resizePositionsFrozen = shouldBeFrozen;
        if (graph.isValid ())
            graph.setProperty (Tags::staticPos, resizePositionsFrozen);
    }

    void changeListenerCallback (ChangeBroadcaster*) override;

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDown (const MouseEvent& e) override;


    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDropped (const SourceDetails& details) override;
    bool shouldDrawDragImageWhenOver() override { return true; }

protected:
    virtual Component* wrapAudioProcessorEditor (AudioProcessorEditor* ed, GraphNodePtr editorNode);
    void createNewPlugin (const PluginDescription* desc, int x, int y);
    
private:
    friend class ConnectorComponent;
    friend class BlockComponent;
    friend class PortComponent;

    Node graph;
    ValueTree data;
    bool resizePositionsFrozen = false;

    float lastDropX = 0.5f;
    float lastDropY = 0.5f;

    std::unique_ptr<ConnectorComponent> draggingConnector;
    std::unique_ptr<BlockFactory> factory;

    bool verticalLayout = true;
    
    SelectedItemSet<uint32> selectedNodes;
    bool ignoreNodeSelected = false;

    void selectNode (const Node& node, ModifierKeys mods);

    Component* createContainerForNode (GraphNodePtr node, bool useGenericEditor);
    AudioProcessorEditor* createEditorForNode (GraphNodePtr node, bool useGenericEditor);
    PluginWindow* getOrCreateWindowForNode (GraphNodePtr f, bool useGeneric);
    
    void updateBlockComponents (const bool doPosition = true);
    void updateConnectorComponents();
    
    void beginConnectorDrag (const uint32 sourceFilterID, const int sourceFilterChannel,
                             const uint32 destFilterID, const int destFilterChannel,
                             const MouseEvent& e);
    void dragConnector (const MouseEvent& e);
    void endDraggingConnector (const MouseEvent& e);
    
    BlockComponent* createBlock (const Node&);

    BlockComponent* getComponentForFilter (const uint32 filterID) const;
    ConnectorComponent* getComponentForConnection (const Arc& conn) const;
    PortComponent* findPinAt (const int x, const int y) const;
    
    void updateSelection();
    
    void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override { }
    void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved,
                                                       int indexFromWhichChildWasRemoved) override { }
    void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved,
                                             int oldIndex, int newIndex) override { }
    void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) override { }
    void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) override { }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphEditorComponent)
};

}
