/*
    DummyTest.cpp - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"

class DummyTest : public UnitTest
{
public:
    DummyTest() : UnitTest ("dummy") { }
    virtual ~DummyTest() { }

    void runTest()
    {
        expect (true);
    }
};

static DummyTest sDummyTest;
