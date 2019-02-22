/*
    SessionDocument.h - This file is part of Element
    Copyright (C) 2019 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "session/Node.h"

namespace Element {

class GraphDocument :  public FileBasedDocument,
                       private ValueTree::Listener
{
public:
    GraphDocument();
    ~GraphDocument();

    inline Node getGraph() const { return graph; }
    
    inline void setGraph (const Node& newGraph)
    {
        ScopedChangeStopper freeze (*this, false);
        data.removeListener (this);
        graph = newGraph;
        data  = graph.getValueTree();
        data.addListener (this);
    }

    String getDocumentTitle() override;
    Result loadDocument (const File& file) override;
    Result saveDocument (const File& file) override;
    File getLastDocumentOpened() override;
    void setLastDocumentOpened (const File& file) override;

    class ScopedChangeStopper
    {
    public:
        ScopedChangeStopper() = delete;
        explicit ScopedChangeStopper (GraphDocument& d, bool statusToSet = false)
            : doc (d), changeStatus (statusToSet)
        {
            doc.listenForChanges = false; 
            doc.setChangedFlag (changeStatus);
        }
        
        ~ScopedChangeStopper() noexcept
        {
            doc.setChangedFlag (changeStatus); 
            doc.listenForChanges = true; 
        }
    
    private:
        GraphDocument& doc;
        const bool changeStatus;
    };

private:
    Node graph;
    ValueTree data;
    File lastGraph;
    bool listenForChanges = true;

    friend class juce::ValueTree;
    inline void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override
    {
        if (! listenForChanges)
            return;
        ignoreUnused (tree, property);
        setChangedFlag (true);
    }

    void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override 
    {
        if (! listenForChanges)
            return;
        ignoreUnused (parent, child);
        setChangedFlag (true);
    }

    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int index) override 
    {
        if (! listenForChanges)
            return;
        ignoreUnused (parent, child, index);
        setChangedFlag (true);
    }

    void valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex) override
    {
        if (! listenForChanges)
            return;
        ignoreUnused (parent, oldIndex, newIndex);
        setChangedFlag (true);
    }

    void valueTreeParentChanged (ValueTree& tree) override
    {
        if (! listenForChanges)
            return;
        ignoreUnused (tree);
    }

    void valueTreeRedirected (ValueTree&) override { }
};

}
