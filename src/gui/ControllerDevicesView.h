/*
    ControllerDevice.cpp - This file is part of Element
    Copyright (C) 2018  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "gui/ContentComponent.h"

namespace Element {

class ControllerDevicesView : public ContentView
{
public:
    ControllerDevicesView();
    virtual ~ControllerDevicesView();

    void resized() override;
    void stabilizeContent() override;
    
private:
    class Content;
    std::unique_ptr<Content> content;
};

class ControllerMapsView : public ContentView
{
public:
    ControllerMapsView();
    virtual ~ControllerMapsView();
};

}
