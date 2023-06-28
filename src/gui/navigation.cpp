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

#include "gui/datapathbrowser.hpp"

#include <element/context.hpp>

#include "gui/views/NodeEditorContentView.h"
#include "gui/views/GraphSettingsView.h"
#include "gui/views/NodeMidiContentView.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/AudioIOPanelView.h"
#include "gui/SessionTreePanel.h"
#include "gui/ViewHelpers.h"
#include <element/ui/style.hpp>
#include <element/ui/commands.hpp>

#include <element/ui/navigation.hpp>

namespace element {

class NavigationConcertinaPanel::Header : public Component
{
public:
    Header (NavigationConcertinaPanel& _parent, Component& _panel)
        : parent (_parent), panel (_panel)
    {
        setInterceptsMouseClicks (false, true);
    }

    virtual ~Header() {}

    virtual void resized() override {}
    virtual void paint (Graphics& g) override
    {
        getLookAndFeel().drawConcertinaPanelHeader (
            g, getLocalBounds(), false, false, parent, panel);
    }

protected:
    NavigationConcertinaPanel& parent;
    Component& panel;
};

class NavigationConcertinaPanel::ElementsHeader : public Header,
                                                  public Button::Listener
{
public:
    ElementsHeader (NavigationConcertinaPanel& _parent, Component& _panel)
        : Header (_parent, _panel)
    {
        addAndMakeVisible (addButton);
        addButton.setIcon (Icon (getIcons().falBars, Colours::white));
        addButton.setTriggeredOnMouseDown (true);
        addButton.addListener (this);
        setInterceptsMouseClicks (false, true);
    }

    void resized() override
    {
        const int padding = 5;
        const int buttonSize = getHeight() - (padding * 2);
        addButton.setBounds (getWidth() - padding - buttonSize,
                             padding,
                             buttonSize,
                             buttonSize);
    }

    static void menuInvocationCallback (int chosenItemID, ElementsHeader* header)
    {
        if (chosenItemID > 0 && header)
            header->menuCallback (chosenItemID);
    }

    void menuCallback (int menuId)
    {
        if (1 == menuId)
        {
            ViewHelpers::invokeDirectly (this, Commands::showSessionConfig, true);
        }
        else if (2 == menuId)
        {
            ViewHelpers::invokeDirectly (this, Commands::sessionAddGraph, true);
        }
    }

    void buttonClicked (Button*) override
    {
        PopupMenu menu;
        menu.addItem (1, "Session Settings...");
        menu.addSeparator();
        menu.addItem (2, "Add Graph");
        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&addButton),
                            ModalCallbackFunction::forComponent (menuInvocationCallback, this));
    }

private:
    IconButton addButton;
};

//====
class NavigationConcertinaPanel::UserDataPathHeader : public Header,
                                                      public Button::Listener
{
public:
    UserDataPathHeader (NavigationConcertinaPanel& _parent, DataPathTreeComponent& _panel)
        : Header (_parent, _panel), tree (_panel)
    {
        addAndMakeVisible (addButton);
        addButton.setButtonText ("+");
        addButton.addListener (this);
        addButton.setTriggeredOnMouseDown (true);
        setInterceptsMouseClicks (false, true);
    }

    void resized() override
    {
        const int padding = 4;
        const int buttonSize = getHeight() - (padding * 2);
        addButton.setBounds (getWidth() - padding - buttonSize,
                             padding,
                             buttonSize,
                             buttonSize);
    }

    void buttonClicked (Button*) override
    {
        PopupMenu menu;
        menu.addItem (1, "Refresh...");
        menu.addSeparator();
#if JUCE_MAC
        String name = "Show in Finder";
#else
        String name = "Show in Explorer";
#endif
        menu.addItem (2, name);
        const int res = menu.show();
        if (res == 1)
        {
            tree.refresh();
        }
        else if (res == 2)
        {
            File file = tree.getSelectedFile();
            if (! file.exists())
                file = file.getParentDirectory();
            if (! file.exists())
                file = tree.getDirectory();
            if (file.exists())
                file.revealToUser();
        }
    }

private:
    DataPathTreeComponent& tree;
    TextButton addButton;
};

//====
NavigationConcertinaPanel::NavigationConcertinaPanel (Context& g)
    : globals (g), headerHeight (22), defaultPanelHeight (80)
{
}

NavigationConcertinaPanel::~NavigationConcertinaPanel()
{
    clearPanels();
    setLookAndFeel (nullptr);
}

Component* NavigationConcertinaPanel::findPanelByName (const String& name)
{
    for (int i = 0; i < getNumPanels(); ++i)
        if (getPanel (i)->getName() == name)
            return getPanel (i);
    return 0;
}

void NavigationConcertinaPanel::saveState (PropertiesFile* props)
{
    ValueTree state (tags::state);

    for (int i = 0; i < getNumPanels(); ++i)
    {
        ValueTree item ("item");
        auto* const panel = getPanel (i);
        item.setProperty ("index", i, 0)
            .setProperty ("name", panel->getName(), 0)
            .setProperty ("h", panel->getHeight(), 0);

        if (auto* ned = dynamic_cast<NodeEditorContentView*> (panel))
            item.setProperty ("sticky", ned->isSticky(), nullptr);

        state.addChild (item, -1, 0);
    }

    if (auto xml = state.createXml())
        props->setValue ("ccNavPanel", xml.get());
}

void NavigationConcertinaPanel::restoreState (PropertiesFile* props)
{
    if (auto xml = props->getXmlValue ("ccNavPanel"))
    {
        ValueTree state = ValueTree::fromXml (*xml);
        for (int i = 0; i < state.getNumChildren(); ++i)
        {
            auto item (state.getChild (i));
            if (auto* c = findPanelByName (item["name"].toString().trim()))
            {
                setPanelSize (c, jmax (10, (int) item["h"]), false);
                if (auto* ned = dynamic_cast<NodeEditorContentView*> (c))
                    ned->setSticky ((bool) item.getProperty ("sticky", ned->isSticky()));
            }
        }
    }
}

int NavigationConcertinaPanel::getIndexOfPanel (Component* panel)
{
    if (nullptr == panel)
        return -1;
    for (int i = 0; i < getNumPanels(); ++i)
        if (auto* p = getPanel (i))
            if (p == panel)
                return i;
    return -1;
}

//====
void NavigationConcertinaPanel::showPanel (const juce::String& name)
{
    namesHidden.removeString (name);
    updateContent();
}

void NavigationConcertinaPanel::hidePanel (const juce::String& name)
{
    namesHidden.addIfNotAlreadyThere (name);
    updateContent();
}

void NavigationConcertinaPanel::setPanelName (const String& panel, const String& newName)
{
    if (auto ptr = findPanelByName (panel))
    {
        ptr->setName (newName);
        repaint();
    }
}

void NavigationConcertinaPanel::clearPanels()
{
    for (int i = 0; i < comps.size(); ++i)
        removePanel (comps.getUnchecked (i));
    comps.clearQuick (true);
}

void NavigationConcertinaPanel::updateContent()
{
    clearPanels();

    if (! namesHidden.contains ("Session"))
    {
        auto* sess = new SessionTreePanel();
        sess->setName ("Session");
        sess->setComponentID ("Session");
        addPanelInternal (-1, sess, "Session", new ElementsHeader (*this, *sess));
    }

    if (! namesHidden.contains ("Graph"))
    {
        auto* gv = new GraphSettingsView();
        gv->setName ("Graph");
        gv->setComponentID ("Graph");
        gv->setGraphButtonVisible (false);
        gv->setUpdateOnActiveGraphChange (true);
        gv->setPropertyPanelHeaderVisible (false);
        addPanelInternal (-1, gv, "Graph", nullptr);
    }

    if (! namesHidden.contains ("Node"))
    {
        auto* nv = new NodeEditorContentView();
        nv->setName ("Node");
        nv->setComponentID ("Node");
        addPanelInternal (-1, nv, "Node", nullptr);
    }

    if (! namesHidden.contains ("MIDI"))
    {
        auto* mv = new NodeMidiContentView();
        mv->setName ("MIDI");
        mv->setComponentID ("MIDI");
        addPanelInternal (-1, mv, "MIDI", nullptr);
    }

    if (! namesHidden.contains ("Plugins"))
    {
        auto* pv = new PluginsPanelView (ViewHelpers::getGlobals (this)->plugins());
        pv->setName ("Plugins");
        pv->setComponentID ("Plugins");
        addPanelInternal (-1, pv, "Plugins", 0);
    }

    if (! namesHidden.contains ("User Data Path"))
    {
        auto* dp = new DataPathTreeComponent();
        dp->setName ("UserDataPath");
        dp->setComponentID ("UserDataPath");
        dp->getFileTree().setDragAndDropDescription ("ccNavConcertinaPanel");
        addPanelInternal (-1, dp, "User Data Path", new UserDataPathHeader (*this, *dp));
    }
}

const StringArray& NavigationConcertinaPanel::getNames() const { return names; }
const int NavigationConcertinaPanel::getHeaderHeight() const { return headerHeight; }

void NavigationConcertinaPanel::setHeaderHeight (const int newHeight)
{
    if (newHeight == headerHeight)
        return;
    jassert (newHeight > 0);
    headerHeight = newHeight;
    for (auto* c : comps)
        setPanelHeaderSize (c, headerHeight);
}

void NavigationConcertinaPanel::addPanelInternal (const int index,
                                                  Component* comp,
                                                  const String& name,
                                                  Component* header)
{
    jassert (comp);
    if (name.isNotEmpty())
        comp->setName (name);
    addPanel (index, comps.insert (index, comp), false);
    setPanelHeaderSize (comp, headerHeight);

    if (nullptr == header)
        header = new Header (*this, *comp);
    setCustomPanelHeader (comp, header, true);
}

void NavigationConcertinaPanel::paint (juce::Graphics& g)
{
    g.fillAll (element::Colors::backgroundColor);
}

} // namespace element
