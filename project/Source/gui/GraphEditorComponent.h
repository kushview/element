/*
    GraphEditorComponent.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "engine/GraphProcessor.h"

namespace Element {

class ConnectorComponent;
class FilterComponent;
class PinComponent;
class PluginWindow;

/** A panel that displays and edits a GraphProcessor. */
class GraphEditorComponent   : public Component,
                               public ChangeListener,
                               public DragAndDropTarget,
                               private ValueTree::Listener
{
public:
    GraphEditorComponent();
    virtual ~GraphEditorComponent();

    void setNode (const Node& n);
    
    void paint (Graphics& g) override;
    void resized() override;
    void mouseDown (const MouseEvent& e) override;

    void changeListenerCallback (ChangeBroadcaster*) override;
    void onGraphChanged();
    void updateComponents();


    bool isInterestedInDragSource (const SourceDetails& /*details*/) override;
    // void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    // void itemDragMove (const SourceDetails& dragSourceDetails) override;
    // void itemDragExit (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& details) override;
    bool shouldDrawDragImageWhenOver() override { return true; }

protected:
    virtual Component* wrapAudioProcessorEditor (AudioProcessorEditor* ed, GraphNodePtr editorNode);
    void createNewPlugin (const PluginDescription* desc, int x, int y);
    
private:
    Node graph;
    ValueTree data;
    friend class ConnectorComponent;
    friend class FilterComponent;
    friend class PinComponent;
    ScopedPointer<ConnectorComponent> draggingConnector;
    
    Component* createContainerForNode (GraphNodePtr node, bool useGenericEditor);
    AudioProcessorEditor* createEditorForNode (GraphNodePtr node, bool useGenericEditor);
    PluginWindow* getOrCreateWindowForNode (GraphNodePtr f, bool useGeneric);
    
    void updateFilterComponents();
    void updateConnectorComponents();
    
    void beginConnectorDrag (const uint32 sourceFilterID, const int sourceFilterChannel,
                             const uint32 destFilterID, const int destFilterChannel,
                             const MouseEvent& e);
    void dragConnector (const MouseEvent& e);
    void endDraggingConnector (const MouseEvent& e);
    
    FilterComponent* getComponentForFilter (const uint32 filterID) const;
    ConnectorComponent* getComponentForConnection (const Arc& conn) const;
    PinComponent* findPinAt (const int x, const int y) const;
    
    
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
