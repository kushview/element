/*
    SessionDocument.h - This file is part of Element
    Copyright (C) 2019 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "session/Node.h"

namespace Element {

class GraphDocument :  public FileBasedDocument
{
public:
    GraphDocument();
    ~GraphDocument();

    inline Node getGraph() const { return graph; }

    String getDocumentTitle() override;
    Result loadDocument (const File& file) override;
    Result saveDocument (const File& file) override;
    File getLastDocumentOpened() override;
    void setLastDocumentOpened (const File& file) override;

private:
    Node graph;
    File lastGraph;
};

}
