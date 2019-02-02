/*
    Workspace.cpp - This file is part of Element
 */

#if EL_DOCKING

#include "Workspace.h"

namespace Element {


class GenericDockPanel : public DockPanel
{
public:
    GenericDockPanel (const String& panelName) 
        : DockPanel ("GenericDockPanel")
    { 
        setName (panelName);
    }
    ~GenericDockPanel() { }

    void showPopupMenu() override
    {
        PopupMenu menu;
        menu.addItem (1, "Close Panel");
        menu.addItem (2, "Undock Panel");
        const auto result = menu.show();

        switch (result)
        {
            case 1: {
                close();
            } break;

            case 2: {
                undock();
            } break;

            default: break;
        }
    }
};

class GenericPanelType : public DockPanelType
{
public:
    int lastPanelNo = 0;
    static const Identifier genericType;

    void getAllTypes (OwnedArray<DockPanelInfo>& types) override
    {
        auto* type = types.add (new DockPanelInfo());
        type->identifier = genericType;
        type->name = "Generic";
        type->description = "A generic panel for development purposes";
    }

    DockPanel* createPanel (const Identifier& panelType) override
    {
        if (panelType == genericType)
        {
            ++lastPanelNo;
            return new GenericDockPanel (String("Generic ") + String(lastPanelNo));
        }
        return nullptr;
    }
};

const Identifier GenericPanelType::genericType = "GenericDockPanel";

Workspace::Workspace()
{
    dock.registerPanelType (new GenericPanelType());
    addAndMakeVisible (dock);
    dock.createItem (GenericPanelType::genericType.toString(), DockPlacement::Top);
    dock.createItem (GenericPanelType::genericType.toString(), DockPlacement::Top);
    dock.createItem (GenericPanelType::genericType.toString(), DockPlacement::Top);
    dock.createItem (GenericPanelType::genericType.toString(), DockPlacement::Top);
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
