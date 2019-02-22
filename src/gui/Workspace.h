/*
    Workspace.h - This file is part of Element
    Copyright (C) 2016-2018 Kushview, LLC.  All rights reserved.
*/

#if EL_DOCKING

#pragma once

#include "ElementApp.h"

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

    // inline Uuid getUuid() const
    // {
    //     const Uuid uuid (getProperty(Tags::uuid).toString());
    //     return uuid;
    // }

    inline String getName() const { return getProperty(Tags::name).toString(); }
    
    inline void applyTo (Dock& dock) const
    {
        auto dockState = objectData.getChildWithName (Tags::dock);
        if (dockState.isValid())
            dock.applyState (dockState);
    }

    bool writeToFile (const File&) const;
    
    static WorkspaceState fromFile (const File&);

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
