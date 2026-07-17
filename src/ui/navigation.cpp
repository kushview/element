// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include <element/context.hpp>
#include <element/ui/style.hpp>
#include <element/ui/commands.hpp>
#include <element/ui/navigation.hpp>

#include "ui/datapathbrowser.hpp"
#include "ui/nodeeditorview.hpp"
#include "ui/graphsettingsview.hpp"
#include "ui/nodepropertiesview.hpp"
#include "ui/pluginspanelview.hpp"
#include "ui/audioiopanelview.hpp"
#include "ui/sessiontreepanel.hpp"
#include "ui/viewhelpers.hpp"

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
        const int padding = 2;
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
    registerBuiltInPanels();
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

        if (auto* ned = dynamic_cast<NodeEditorView*> (panel))
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
        std::vector<ValueTree> withSize;
        for (int i = 0; i < state.getNumChildren(); ++i)
        {
            auto item (state.getChild (i));
            const auto h = std::max (0, (int) item["h"]);
            if (auto* c = findPanelByName (item["name"].toString().trim()))
            {
                if (auto* ned = dynamic_cast<NodeEditorView*> (c))
                    ned->setSticky ((bool) item.getProperty ("sticky", ned->isSticky()));

                if (h > 0)
                {
                    withSize.push_back (item);
                    continue;
                }

                setPanelSize (c, 0, false);
            }
        }

        for (const auto& item : withSize)
            if (auto* c = findPanelByName (item["name"].toString().trim()))
                setPanelSize (c, (int) item["h"], false);
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

void NavigationConcertinaPanel::insertPanel (juce::Component* comp, int index)
{
    addPanelInternal (index, comp);
}

void NavigationConcertinaPanel::showPanel (const juce::String& name)
{
    setPanelHidden (name, false);
}

void NavigationConcertinaPanel::hidePanel (const juce::String& name)
{
    setPanelHidden (name, true);
}

void NavigationConcertinaPanel::setPanelHidden (const juce::String& name, bool hidden)
{
    bool changed = false;
    for (auto& entry : registered)
    {
        if (entry.desc.name == name && entry.desc.hidden != hidden)
        {
            entry.desc.hidden = hidden;
            changed = true;
        }
    }

    if (changed)
        updateContent();
}

void NavigationConcertinaPanel::setPanelName (const String& panel, const String& newName)
{
    for (auto& entry : registered)
        if (entry.desc.name == panel)
            entry.desc.name = newName;

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

void NavigationConcertinaPanel::registerBuiltInPanels()
{
    // Register only; updateContent() creates them. Going through addPanel() here
    // would construct each panel eagerly, only for the first updateContent() to
    // clear and rebuild them.
    auto reg = [this] (PanelDescription desc, PanelFactory factory, PanelHeaderFactory header = nullptr) {
        registered.push_back ({ std::move (desc), std::move (factory), std::move (header) });
    };

    reg (
        { "Session" },
        [] { return new SessionTreePanel(); },
        [this] (Component& panel) { return new ElementsHeader (*this, panel); });

    reg ({ "Graph" }, [] {
        auto* gv = new GraphSettingsView();
        gv->setGraphButtonVisible (false);
        gv->setUpdateOnActiveGraphChange (true);
        gv->setPropertyPanelHeaderVisible (false);
        return gv;
    });

    reg ({ "Node" }, [] { return new NodePropertiesView(); });

    reg ({ "Editor" }, [] { return new NodeEditorView(); });

    reg ({ "Plugins" }, [this] { return new PluginsPanelView (globals.plugins()); });

    reg (
        { "Data Path", "UserDataPath" },
        [] {
            auto* dp = new DataPathTreeComponent();
            dp->getFileTree().setDragAndDropDescription ("ccNavConcertinaPanel");
            return dp;
        },
        [this] (Component& panel) {
            return new UserDataPathHeader (*this, static_cast<DataPathTreeComponent&> (panel));
        });
}

void NavigationConcertinaPanel::updateContent()
{
    clearPanels();
    createRegisteredPanels();
}

void NavigationConcertinaPanel::createRegisteredPanels()
{
    for (const auto& entry : registered)
        createPanel (entry);
}

void NavigationConcertinaPanel::createPanel (const RegisteredPanel& entry)
{
    if (entry.desc.hidden || ! entry.factory)
        return;

    auto* comp = entry.factory();
    if (comp == nullptr)
        return;

    comp->setComponentID (entry.desc.componentID.isNotEmpty() ? entry.desc.componentID
                                                              : entry.desc.name);

    Component* header = entry.header ? entry.header (*comp) : nullptr;
    addPanelInternal (entry.desc.index, comp, entry.desc.name, header);

    if (entry.desc.preferredSize > 0)
        setPanelSize (comp, entry.desc.preferredSize, false);
}

void NavigationConcertinaPanel::addPanel (PanelDescription desc, PanelFactory factory, PanelHeaderFactory header)
{
    if (! factory || findPanelByName (desc.name) != nullptr)
        return;

    registered.push_back ({ std::move (desc), std::move (factory), std::move (header) });
    createPanel (registered.back());
}

void NavigationConcertinaPanel::removePanel (const String& name)
{
    registered.erase (std::remove_if (registered.begin(),
                                      registered.end(),
                                      [&name] (const RegisteredPanel& p) { return p.desc.name == name; }),
                      registered.end());

    if (auto* comp = findPanelByName (name))
    {
        ConcertinaPanel::removePanel (comp);
        comps.removeObject (comp);
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
