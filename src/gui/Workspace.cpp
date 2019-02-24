/*
    Workspace.cpp - This file is part of Element
 */

#if EL_DOCKING

#include "ElementApp.h"
#include "controllers/AppController.h"
#include "gui/workspace/PanelTypes.h"
#include "gui/workspace/WorkspacePanel.h"
#include "gui/Workspace.h"

namespace Element {

WorkspaceState::WorkspaceState ()
    : ObjectModel (Tags::workspace)
{
    setMissing();
}

WorkspaceState::WorkspaceState (Workspace& w, const String& name)
    : ObjectModel (Tags::workspace)
{
    setMissing();
    if (name.isNotEmpty())
        objectData.setProperty (Tags::name, name, nullptr);
    objectData.appendChild (w.getDock().getState(), nullptr);
}

bool WorkspaceState::writeToFile (const File& file) const
{
    TemporaryFile tempFile (file);
    bool succeded = false;
    
    if (auto out = std::unique_ptr<FileOutputStream> (tempFile.getFile().createOutputStream()))
    {
        GZIPCompressorOutputStream gzip (*out);
        objectData.writeToStream (gzip);
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
    
    if (tryXml)
    {
        if (auto xml = std::unique_ptr<XmlElement> (XmlDocument::parse (file)))
        {
            state.objectData = ValueTree::fromXml (*xml);
            if (state.objectData.hasType (Tags::workspace))
                return state;
        }
    }

    if (auto in = std::unique_ptr<FileInputStream> (file.createInputStream()))
    {
        GZIPDecompressorInputStream gzip (*in);
        state.objectData = ValueTree::readFromStream (gzip);
    }

    return state;
}

WorkspaceState WorkspaceState::fromXmlFile (const File& file)
{
    WorkspaceState state;
    if (auto xml = std::unique_ptr<XmlElement> (XmlDocument::parse (file)))
        state.objectData = ValueTree::fromXml (*xml);
    return state;
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
    WorkspaceState state (*this);
    return state;
}

void Workspace::applyState (const WorkspaceState& state)
{
    state.applyTo (dock);
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

#endif
