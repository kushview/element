/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "ElementApp.h"

#define EL_WORKSPACE_CLASSIC "Classic"
#define EL_WORKSPACE_EDITING "Editing"
#define EL_WORKSPACE_MIXING "Mixing"

namespace element {

class ServiceManager;
class GuiService;
class Context;

class Workspace;

class WorkspaceState : public ObjectModel
{
public:
    WorkspaceState();
    WorkspaceState (Workspace&, const String& name = String());
    ~WorkspaceState() = default;

    inline String getName() const { return getProperty (Tags::name).toString(); }
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
    Workspace (Context&, ServiceManager&, GuiService&);
    virtual ~Workspace();

    Dock& getDock();

    WorkspaceState getState();
    void applyState (const WorkspaceState& state);

    void paint (Graphics& g) override;
    void resized() override;

private:
    Dock dock;
    Context& world;
    ServiceManager& app;
    GuiService& gui;
};

} // namespace element
