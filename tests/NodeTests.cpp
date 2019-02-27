/*
    DummyTest.cpp - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"

using namespace Element;

namespace Element {

class NodeTests : public UnitTestBase
{
public:
    explicit NodeTests (const String& name = "Node Test")
        : UnitTestBase (name, "nodes", "nodes") { }
    virtual ~NodeTests() { }

    void runTest() override
    {
        testDefaultGraph();
    }

private:
    void testDefaultGraph()
    {
        beginTest ("Default Graph");
        auto node = Node::createDefaultGraph ("CustomName");
        expect (node.getName() == "CustomName");
        expect (node.getNumNodes() == 4);
        for (int i = 0; i < node.getNumNodes(); ++i)
            expect (! node.getNode(i).getUuid().isNull());

        beginTest ("Default Graph No Name");
        node = Node::createDefaultGraph();
        expect (node.getName().isEmpty());
    }
};

static NodeTests sNodeTests;

}
