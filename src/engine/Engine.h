/*
    Engine.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"

namespace Element {

class Engine : public ReferenceCountedObject
{
public:
    virtual ~Engine() { }
    virtual AudioIODeviceCallback& getAudioIODeviceCallback() = 0;
    virtual MidiInputCallback& getMidiInputCallback() = 0;
};

typedef ReferenceCountedObjectPtr<Engine> EnginePtr;

}
