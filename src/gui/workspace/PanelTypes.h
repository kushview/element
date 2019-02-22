#pragma once

#include "ElementApp.h"

namespace Element {

namespace PanelIDs
{
    static const Identifier virtualKeyboard     = "virtualKeyboard";
    static const Identifier graphMixer          = "graphMixer";
    static const Identifier graphEditor         = "graphEditor";
    static const Identifier nodeEditor          = "nodeEditor";
    static const Identifier sessionSettings     = "sessionSettings";
    static const Identifier nodeChannelStrip    = "nodeChannelStrip";
    static const Identifier controllerDevices   = "controllerDevives";
}

/** A generic panel used for testing and development */
class GenericPanelType : public DockPanelType
{
public:
    int lastPanelNo = 0;
    static const Identifier genericType;

    inline void getAllTypes (OwnedArray<DockPanelInfo>& types) override
    {
        auto* type = types.add (new DockPanelInfo());
        type->identifier = genericType;
        type->name = "Generic";
        type->description = "A generic panel for development purposes";
    }

    DockPanel* createPanel (const Identifier& panelType) override;
};

/** Application level panels */
class ApplicationPanelType : public DockPanelType
{
public:
    int lastPanelNo = 0;
    static const Identifier genericType;

    inline void getAllTypes (OwnedArray<DockPanelInfo>& types) override
    {
        auto* type = types.add (new DockPanelInfo());
        type->identifier    = PanelIDs::virtualKeyboard;
        type->name          = "Virtual Keyboard";
        type->description   = "Embedded virtual keyboard which sends MIDI events to the Global MIDI input";

        type = types.add (new DockPanelInfo());
        type->identifier    = PanelIDs::graphMixer;
        type->name          = "Graph Mixer";
        type->description   = "A mixer where the channel strips represent a node on a graph";

        type = types.add (new DockPanelInfo());
        type->identifier    = PanelIDs::graphEditor;
        type->name          = "Graph Editor";
        type->description   = "The Graph Editor";

        type = types.add (new DockPanelInfo());
        type->identifier    = PanelIDs::nodeEditor;
        type->name          = "Node Editor";
        type->description   = "The Node Editor";

        type = types.add (new DockPanelInfo());
        type->identifier    = PanelIDs::nodeChannelStrip;
        type->name          = "Node Channel Strip";
        type->description   = "Displays a single channel strip for a given node";
    }

    DockPanel* createPanel (const Identifier& panelType) override;
};

};
