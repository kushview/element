/*
    Workspace.cpp - This file is part of Element
*/

#if EL_DOCKING

#include "Workspace.h"

namespace Element {

Workspace::Workspace()
{
    addAndMakeVisible (dock);
    setSize (1280, 640);
}

Workspace::~Workspace()
{
}

Dock& Workspace::getDock() { return dock; }

void Workspace::setMainComponent (Component* c)
{
}

void Workspace::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void Workspace::mouseDown (const MouseEvent& /*ev*/)
{

}

void Workspace::resized()
{
    dock.setBounds (getLocalBounds());
}

}

#endif
