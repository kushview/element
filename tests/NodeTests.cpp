/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
