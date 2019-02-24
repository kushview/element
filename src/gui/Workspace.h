/*
    Workspace.h - This file is part of Element
    Copyright (C) 2016-2018 Kushview, LLC.  All rights reserved.
*/

#if EL_DOCKING

#pragma once

#include "ElementApp.h"

#define EL_WORKSPACE_CLASSIC "Classic"
#define EL_WORKSPACE_EDITING "Editing"

namespace Element {

class AppController;
class GuiController;
class Globals;

class Workspace;

class WorkspaceState : public ObjectModel
{
public:
    WorkspaceState();
    WorkspaceState (Workspace&, const String& name = String());
    ~WorkspaceState() = default;

    inline String getName() const { return getProperty(Tags::name).toString(); }
    inline bool isValid() const { return objectData.isValid() && objectData.hasType (Tags::workspace); }
    inline void applyTo (Dock& dock) const
    {
        auto dockState = objectData.getChildWithName (Tags::dock);
        if (dockState.isValid())
            dock.applyState (dockState);
    }

    /** Write to binary file */
    bool writeToFile (const File&) const;

    /** Write to human readable XML file */
    bool writeToXmlFile (const File&) const;

    /** Load from binary formatted file. tryXml will try to load from xml first */
    static WorkspaceState fromFile (const File&, bool tryXml = false);

    /** Load explicitly from an xml file. Doesn't check binary format */
    static WorkspaceState fromXmlFile (const File&);

    static WorkspaceState loadByName (const String& name);

    static WorkspaceState loadByFileOrName (const String& name);

    inline WorkspaceState& operator= (const WorkspaceState& o)
    {
        ObjectModel::operator= (o);
        return *this;
    }

private:
    void setMissing();
};

class Workspace : public Component
{
public:
    Workspace (Globals&, AppController&, GuiController&);
    virtual ~Workspace();

    Dock& getDock();

    WorkspaceState getState();
    void applyState (const WorkspaceState& state);

    void paint (Graphics& g) override;
    void resized() override;

private:
    Dock dock;
    Globals& world;
    AppController& app;
    GuiController& gui;
};

}

#endif
