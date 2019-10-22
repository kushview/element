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

#include "ElementApp.h"
#include "controllers/AppController.h"
#include "gui/workspace/PanelTypes.h"
#include "gui/workspace/WorkspacePanel.h"
#include "gui/Workspace.h"

namespace Element {

WorkspaceState::WorkspaceState()
    : ObjectModel()
{ }

WorkspaceState::WorkspaceState (Workspace& w, const String& name)
    : ObjectModel (Tags::workspace)
{
    setMissing();
    if (name.isNotEmpty())
        objectData.setProperty (Tags::name, name, nullptr);
    else
        objectData.setProperty (Tags::name, w.getName(), nullptr);
    objectData.appendChild (w.getDock().getState(), nullptr);
}

bool WorkspaceState::writeToFile (const File& file) const
{
    TemporaryFile tempFile (file);
    bool succeded = false;
    
    if (auto out = std::unique_ptr<FileOutputStream> (tempFile.getFile().createOutputStream()))
    {
        {
            GZIPCompressorOutputStream gzip (*out);
            objectData.writeToStream (gzip);
        }
        out.reset();
        succeded = tempFile.overwriteTargetFileWithTemporary();
    }

    return succeded;
}

bool WorkspaceState::writeToXmlFile (const File& file) const
{
    TemporaryFile tempFile (file);
    bool succeded = false;

    if (auto out = std::unique_ptr<FileOutputStream> (tempFile.getFile().createOutputStream()))
    {
        if (auto xml = std::unique_ptr<XmlElement> (createXml()))
        {
            xml->writeToStream (*out, String());
            out.reset();
            succeded = tempFile.overwriteTargetFileWithTemporary();
        }
    }

    return succeded;
}

WorkspaceState WorkspaceState::fromFile (const File& file, bool tryXml)
{
    WorkspaceState state;
    jassert(! state.isValid());
    DBG("[EL] workspace loading: " << file.getFileName());

    if (tryXml)
    {
        if (auto xml = std::unique_ptr<XmlElement> (XmlDocument::parse (file)))
            state.objectData = ValueTree::fromXml (*xml);
    }

    if (! state.isValid())
    {
        if (auto in = std::unique_ptr<FileInputStream> (file.createInputStream()))
        {
            GZIPDecompressorInputStream gzip (*in);
            state.objectData = ValueTree::readFromStream (gzip);
        }
    }

    if (state.isValid())
    {
        state.objectData.setProperty (
            Tags::name, file.getFileNameWithoutExtension(), nullptr);
    }

    return state;
}

WorkspaceState WorkspaceState::fromXmlFile (const File& file)
{
    WorkspaceState state;
    DBG("[EL] workspace loading: " << file.getFileName());

    if (auto xml = std::unique_ptr<XmlElement> (XmlDocument::parse (file)))
        state.objectData = ValueTree::fromXml (*xml);
    
    if (state.isValid())
    {
        state.objectData.setProperty (
            Tags::name, file.getFileNameWithoutExtension(), nullptr);
    }

    return state;
}

WorkspaceState WorkspaceState::loadByName (const String& name)
{
    WorkspaceState state;
    DBG("[EL] workspace loading: " << name);
    if (name == "Classic")
    {
        if (auto* xml = XmlDocument::parse (String::fromUTF8 (
            BinaryData::Classic_elw, BinaryData::Classic_elwSize)))
        {
            state.objectData = ValueTree::fromXml (*xml);
            delete xml;
        }
    }
    else if (name == "Editing")
    {
        if (auto* xml = XmlDocument::parse (String::fromUTF8 (
            BinaryData::Editing_elw, BinaryData::Editing_elwSize)))
        {
            state.objectData = ValueTree::fromXml (*xml);
            delete xml;
        }
    }
    
    if (state.isValid())
        state.objectData.setProperty (Tags::name, name, nullptr);

    return state;
}

WorkspaceState WorkspaceState::loadByFileOrName (const String& name)
{
    const auto file = DataPath::workspacesDir().getChildFile (name + ".elw");
    if (file.existsAsFile())
        return fromFile (file, true);
    return loadByName (name);
}

void WorkspaceState::setMissing()
{
    stabilizePropertyString (Tags::name, "New Workspace");
    // stabilizePropertyString (Tags::uuid, Uuid().toString());
}

Workspace::Workspace (Globals& w, AppController& a, GuiController& g)
    : world (w), app (a), gui (g)
{
    dock.registerPanelType (new GenericPanelType());
    dock.registerPanelType (new ApplicationPanelType());
    
    dock.onPanelAdded = [this](DockPanel* panel)
    {
        if (auto* const wp = dynamic_cast<WorkspacePanel*> (panel))
        {
            wp->initializeView (app);
            wp->didBecomeActive();
        }
    };

    addAndMakeVisible (dock);
    setSize (1280, 640);
}

Workspace::~Workspace()
{
}

Dock& Workspace::getDock() { return dock; }

WorkspaceState Workspace::getState()
{
    jassert (getName().isNotEmpty());
    WorkspaceState state (*this);
    jassert (state.getName() == getName());
    return state;
}

void Workspace::applyState (const WorkspaceState& state)
{
    if (state.isValid())
    {
        state.applyTo (dock);
        setName (state.getName());
        DBG("[EL] workspace loaded: " << getName());
    }
}

void Workspace::paint (Graphics& g)
{
    g.fillAll (Colours::red);
}

void Workspace::resized()
{
    dock.setBounds (getLocalBounds());
}

}
