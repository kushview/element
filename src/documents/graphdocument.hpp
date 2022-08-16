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
#include "session/node.hpp"
#include "session/session.hpp"

namespace element {

class GraphDocument : public FileBasedDocument,
                      private ValueTree::Listener
{
public:
    GraphDocument();
    ~GraphDocument();

    inline void setSession (SessionPtr newSession)
    {
        ScopedChangeStopper freeze (*this, false);
        jassert (newSession != nullptr);
        session = newSession;
    }

    inline Node getGraph() const { return graph; }

    inline void setGraph (const Node& newGraph)
    {
        jassert (session != nullptr);
        ScopedChangeStopper freeze (*this, false);
        data.removeListener (this);
        graph = newGraph;
        session->clear();
        session->addGraph (graph, true);
        data = session->getValueTree();
        data.addListener (this);
    }

    String getDocumentTitle() override;
    Result loadDocument (const File& file) override;
    Result saveDocument (const File& file) override;
    File getLastDocumentOpened() override;
    void setLastDocumentOpened (const File& file) override;
    File getSuggestedSaveAsFile (const File&) override {
        return getFile().getNonexistentSibling (true);
    }
    
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
    SessionPtr session;
    Node graph;
    ValueTree data;
    File lastGraph;
    bool listenForChanges = true;

    inline void bindChangeHandlers()
    {
        ScopedChangeStopper freeze (*this, false);
        data.removeListener (this);
        graph = session->getActiveGraph();
        data = session->getValueTree();
        data.addListener (this);
    }

    friend class juce::ValueTree;
    inline void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override
    {
        if (! listenForChanges)
            return;
        ignoreUnused (tree, property);
        setChangedFlag (true);
    }

    inline void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override
    {
        if (! listenForChanges)
            return;
        ignoreUnused (parent, child);
        setChangedFlag (true);
    }

    inline void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int index) override
    {
        if (! listenForChanges)
            return;
        ignoreUnused (parent, child, index);
        setChangedFlag (true);
    }

    inline void valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex) override
    {
        if (! listenForChanges)
            return;
        ignoreUnused (parent, oldIndex, newIndex);
        setChangedFlag (true);
    }

    inline void valueTreeParentChanged (ValueTree& tree) override
    {
        if (! listenForChanges)
            return;
        ignoreUnused (tree);
    }

    inline void valueTreeRedirected (ValueTree&) override {}
};

} // namespace element
