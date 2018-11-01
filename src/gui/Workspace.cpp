/*
    Workspace.cpp - This file is part of Element
*/

#if EL_DOCKING

#include "Workspace.h"

namespace Element {

Workspace::Workspace()
{
    addAndMakeVisible (dock = new Dock());

    setSize (1280, 640);

    setMainComponent (new ScreenDisplay());
    dock->getBottomArea().setVisible (false);
}

Workspace::~Workspace()
{
    dock = nullptr;
}

Dock& Workspace::getDock()
{
    jassert (dock != nullptr);
    return *dock;
}

void Workspace::setMainComponent (Component* c)
{
    DockItem* item = dock->createItem ("test", "Test Item", Dock::TopArea);
    item->setContentOwned (c);
    item->setMaximized (true);
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
    Rectangle<int> b (getLocalBounds());
    dock->setBounds (b.reduced (3));
}

}

#endif
