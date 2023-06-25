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

#include <element/services.hpp>
#include <element/ui/meterbridge.hpp>
#include "gui/datapathbrowser.hpp"

#include "services/mappingservice.hpp"
#include "services/sessionservice.hpp"
#include "gui/views/EmptyContentView.h"
#include "gui/AudioIOPanelView.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/ConnectionGrid.h"
#include "gui/views/ControllersView.h"
#include "gui/views/GraphEditorView.h"
#include "gui/views/GraphMixerView.h"
#include "gui/views/KeymapEditorView.h"
#include "gui/views/LuaConsoleView.h"
#include "gui/views/NodeChannelStripView.h"
#include "gui/MainWindow.h"
#include "gui/MainMenu.h"
#include "gui/widgets/MidiBlinker.h"
#include "gui/views/NavigationView.h"
#include "gui/SessionTreePanel.h"
#include "gui/ViewHelpers.h"
#include <element/ui/style.hpp>
#include "gui/PluginManagerComponent.h"
#include "gui/views/SessionSettingsView.h"
#include "gui/views/GraphSettingsView.h"
#include "gui/views/VirtualKeyboardView.h"
#include "gui/TempoAndMeterBar.h"
#include "gui/TransportBar.h"
#include <element/ui/navigation.hpp>
#include "gui/views/NodeEditorContentView.h"
#include "gui/views/GraphSettingsView.h"
#include "gui/views/NodeMidiContentView.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/AudioIOPanelView.h"
#include "gui/SessionTreePanel.h"

#include <element/devices.hpp>
#include <element/plugins.hpp>
#include <element/node.hpp>

#include <element/ui/commands.hpp>
#include <element/context.hpp>
#include <element/settings.hpp>

#include <element/ui/standard.hpp>

#include "BinaryData.h"

#ifndef EL_USE_ACCESSORY_BUTTONS
#define EL_USE_ACCESSORY_BUTTONS 0
#endif

#define EL_NAV_MIN_WIDTH 100
#define EL_NAV_MAX_WIDTH 440

namespace element {

//=============================================================================
#if 0
class GraphEditorViewSE : public GraphEditorView
{
public:
    GraphEditorViewSE()
    {
        logo1 = ImageCache::getFromMemory (BinaryData::kushviewlogotext_220_png,
                                           BinaryData::kushviewlogotext_220_pngSize);
        logo2 = ImageCache::getFromMemory (BinaryData::digitalelementslogo_220_png,
                                           BinaryData::digitalelementslogo_220_pngSize);
    }

    ~GraphEditorViewSE() = default;

    void paintOverChildren (Graphics& g) override
    {
        const int pad = 10;
        auto dw = logo1.getWidth() * 0.38;
        auto dh = logo1.getHeight() * 0.38;
        g.drawImageWithin (logo1, -40, getHeight() - dh - pad, dw, dh, RectanglePlacement::centred);
        dw = logo2.getWidth() * 0.4;
        dh = logo2.getHeight() * 0.4;
        g.drawImageWithin (logo2, getWidth() - dw - pad, getHeight() - dh - pad, dw, dh, RectanglePlacement::xRight | RectanglePlacement::yBottom);
    }

private:
    Image logo1, logo2;
};
#endif

static GraphEditorView* createGraphEditorView()
{
    return new GraphEditorView();
}

//=============================================================================
class SmartLayoutManager : public StretchableLayoutManager
{
public:
    SmartLayoutManager() {}
    virtual ~SmartLayoutManager() {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartLayoutManager)
};

class SmartLayoutResizeBar : public StretchableLayoutResizerBar
{
public:
    Signal<void()> mousePressed, mouseReleased;
    SmartLayoutResizeBar (StretchableLayoutManager* layoutToUse,
                          int itemIndexInLayout,
                          bool isBarVertical)
        : StretchableLayoutResizerBar (layoutToUse, itemIndexInLayout, isBarVertical)
    {
        mousePressed.disconnect_all_slots();
        mouseReleased.disconnect_all_slots();
    }

    ~SmartLayoutResizeBar()
    {
    }

    void mouseDown (const MouseEvent& ev) override
    {
        StretchableLayoutResizerBar::mouseDown (ev);
        mousePressed();
    }

    void mouseUp (const MouseEvent& ev) override
    {
        StretchableLayoutResizerBar::mouseUp (ev);
        mouseReleased();
    }
};

// MARK: Content container

class ContentContainer : public Component
{
public:
    ContentContainer (StandardContentComponent& cc, Services& app)
        : owner (cc)
    {
        primary.reset (new ContentView());
        addAndMakeVisible (primary.get());
        bar.reset (new SmartLayoutResizeBar (&layout, 1, false));
        addAndMakeVisible (bar.get());
        bar->mousePressed.connect (
            std::bind (&ContentContainer::updateLayout, this));
        bar->mouseReleased.connect (
            std::bind (&ContentContainer::lockLayout, this));
        secondary.reset (new ContentView());
        addAndMakeVisible (primary.get());

        bottom.reset (new Bottom (cc));
        addAndMakeVisible (bottom.get());
        bottom->setVisible (false);

        updateLayout();
        resized();
    }

    virtual ~ContentContainer() {}

    void resized() override
    {
        if (primary == nullptr)
            return;

        auto r = getLocalBounds();
        if (bottom->requiredHeight() > 0)
        {
            bottom->setVisible (true);
            bottom->setBounds (r.removeFromBottom (bottom->requiredHeight()));
            r.removeFromBottom (1);
        }
        else
        {
            bottom->setVisible (false);
        }

        if (showAccessoryView && secondary != nullptr)
        {
            Component* comps[] = {
                primary.get(),
                bar.get(),
                secondary.get()
            };

            layout.layOutComponents (comps, 3, 0, 0, r.getWidth(), r.getHeight(), true, true);
            lastSecondaryHeight = secondary->getHeight();
        }
        else
        {
            primary->setBounds (r);
        }
    }

    void setNode (const Node& node)
    {
        if (auto* gdv = dynamic_cast<GraphDisplayView*> (primary.get()))
            gdv->setNode (node);
        else if (auto* grid = dynamic_cast<ConnectionGrid*> (primary.get()))
            grid->setNode (node);
        else if (auto* ed = dynamic_cast<GraphEditorView*> (primary.get()))
            ed->setNode (node);
        else if (nullptr != primary)
            primary->stabilizeContent();
    }

    void setMainView (ContentView* view)
    {
        if (view)
            view->initializeView (owner.services());

        if (primary)
        {
            primary->willBeRemoved();
            removeChildComponent (primary.get());
        }

        primary.reset (view);

        if (primary)
        {
            primary->willBecomeActive();
            addAndMakeVisible (primary.get());
        }

        resized();

        if (primary)
        {
            primary->didBecomeActive();
            primary->stabilizeContent();
        }
    }

    void setSecondaryView (ContentView* view)
    {
        if (view)
            view->initializeView (owner.services());

        if (secondary)
        {
            secondary->willBeRemoved();
            removeChildComponent (secondary.get());
        }

        secondary.reset (view);

        if (secondary)
        {
            secondary->willBecomeActive();
            addAndMakeVisible (secondary.get());
        }

        // TODO: Improve this logic.
        bool shouldShow = secondary != nullptr;
        showAccessoryView = ! shouldShow;
        setShowAccessoryView (shouldShow);

        if (secondary)
        {
            secondary->didBecomeActive();
            secondary->stabilizeContent();
        }
    }

    void setShowAccessoryView (const bool show)
    {
        if (showAccessoryView == show)
            return;
        showAccessoryView = show;
        if (showAccessoryView)
        {
            lastSecondaryHeight = jmax (50, lastSecondaryHeight);
            layout.setItemLayout (0, 48, -1.0, primary->getHeight() - 4 - lastSecondaryHeight);
            layout.setItemLayout (1, barSize, barSize, barSize);
            layout.setItemLayout (2, 48, -1.0, lastSecondaryHeight);
        }

        resized();
    }

    void saveState (PropertiesFile* props)
    {
        props->setValue ("ContentContainer_lastSecondaryHeight", lastSecondaryHeight);
    }

    void restoreState (PropertiesFile* props)
    {
        lastSecondaryHeight = props->getIntValue ("ContentContainer_lastSecondaryHeight", 52);
    }

private:
    friend class StandardContentComponent;
    StandardContentComponent& owner;

    StretchableLayoutManager layout;
    std::unique_ptr<ContentView> primary;
    std::unique_ptr<SmartLayoutResizeBar> bar;
    std::unique_ptr<ContentView> secondary;

    class Bottom : public juce::Component
    {
    public:
        StandardContent& standard;
        Bottom (StandardContent& s) : standard (s)
        {
            keyboard = std::make_unique<VirtualKeyboardView>();
            keyboard->willBecomeActive();
            addAndMakeVisible (keyboard.get());
            keyboard->initializeView (standard.services());
            keyboard->didBecomeActive();

            bridge = std::make_unique<MeterBridgeView>();
            bridge->willBecomeActive();
            addAndMakeVisible (bridge.get());
            bridge->initializeView (standard.services());
            bridge->didBecomeActive();
        }

        ~Bottom() {}

        void paint (Graphics& g) override
        {
            g.fillAll (Colours::black);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            if (keyboard->isVisible() && bridge->isVisible())
            {
                keyboard->setBounds (r.removeFromBottom (80));
                r.removeFromBottom (1);
                bridge->setBounds (r.removeFromBottom (80));
            }
            else if (keyboard->isVisible())
                keyboard->setBounds (r.removeFromBottom (80));
            else if (bridge->isVisible())
                bridge->setBounds (r.removeFromBottom (80));
        }

        int requiredHeight()
        {
            int h = keyboard->isVisible() ? 80 : 0;
            h += (bridge->isVisible() ? 80 : 0);
            if (keyboard->isVisible() && bridge->isVisible())
                h += 1;
            return h;
        }

        std::unique_ptr<VirtualKeyboardView> keyboard;
        std::unique_ptr<MeterBridgeView> bridge;
    };

    std::unique_ptr<Bottom> bottom;

    bool showAccessoryView = false;
    int barSize = 2;
    int lastSecondaryHeight = 172;
    int capturedAccessoryHeight = -1;
    const int accessoryHeightThreshold = 10;
    bool locked = true;

    void lockLayout()
    {
        locked = true;

        const int primaryMin = 48;
        if (showAccessoryView)
        {
            layout.setItemLayout (0, primaryMin, -1.0, primary->getHeight());
            layout.setItemLayout (1, barSize, barSize, barSize);
            layout.setItemLayout (2, secondary->getHeight(), secondary->getHeight(), secondary->getHeight());
        }

        resized();

        if (showAccessoryView)
        {
            capturedAccessoryHeight = secondary->getHeight();
        }
    }

    void updateLayout()
    {
        locked = false;

        if (showAccessoryView)
        {
            layout.setItemLayout (0, 48, -1.0, primary->getHeight());
            layout.setItemLayout (1, barSize, barSize, barSize);
            layout.setItemLayout (2, 48, -1.0, secondary->getHeight());
        }

        resized();

        if (showAccessoryView)
        {
            capturedAccessoryHeight = secondary->getHeight();
        }
    }
};

class StandardContentComponent::Resizer : public StretchableLayoutResizerBar
{
public:
    Resizer (StandardContentComponent& StandardContentComponent, StretchableLayoutManager* layoutToUse, int itemIndexInLayout, bool isBarVertical)
        : StretchableLayoutResizerBar (layoutToUse, itemIndexInLayout, isBarVertical),
          owner (StandardContentComponent)
    {
    }

    void mouseDown (const MouseEvent& ev) override
    {
        StretchableLayoutResizerBar::mouseDown (ev);
        owner.resizerMouseDown();
    }

    void mouseUp (const MouseEvent& ev) override
    {
        StretchableLayoutResizerBar::mouseUp (ev);
        owner.resizerMouseUp();
    }

private:
    StandardContentComponent& owner;
};

//==============================================================================
#if 0
static ContentView* createLastContentView (Settings& settings)
{
    auto* props = settings.getUserSettings();
    const String lastContentView = props->getValue ("lastContentView");
    std::unique_ptr<ContentView> view;
    typedef GraphEditorView DefaultView;

    if (lastContentView.isEmpty())
        view = std::make_unique<DefaultView>();
    else if (lastContentView == "PatchBay")
        view = std::make_unique<ConnectionGrid>();
    else if (lastContentView == "GraphEditor")
        view.reset (createGraphEditorView());
    else if (lastContentView == "ControllersView")
        view = std::make_unique<ControllersView>();
    else
        view = std::make_unique<DefaultView>();

    return view ? view.release() : nullptr;
}

static String stringProperty (Settings& settings, const String& property, const String defaultValue = String())
{
    auto* props = settings.getUserSettings();
    return props == nullptr ? String() : props->getValue (property, defaultValue);
}

static bool booleanProperty (Settings& settings, const String& property, const bool defaultValue)
{
    auto* props = settings.getUserSettings();
    return props == nullptr ? false : props->getBoolValue (property, defaultValue);
}

static void windowSizeProperty (Settings& settings, const String& property, int& w, int& h, const int defaultW, const int defaultH)
{
    auto* props = settings.getUserSettings();
    StringArray tokens;
    tokens.addTokens (props->getValue (property).trim(), false);
    tokens.removeEmptyStrings();
    tokens.trim();

    w = defaultW;
    h = defaultH;

    const bool fs = tokens[0].startsWithIgnoreCase ("fs");
    const int firstCoord = fs ? 1 : 0;

    if (tokens.size() != firstCoord + 4)
        return;

    Rectangle<int> newPos (tokens[firstCoord].getIntValue(),
                           tokens[firstCoord + 1].getIntValue(),
                           tokens[firstCoord + 2].getIntValue(),
                           tokens[firstCoord + 3].getIntValue());

    if (! newPos.isEmpty())
    {
        w = newPos.getWidth();
        h = newPos.getHeight();
    }
}
#endif

//==============================================================================
StandardContentComponent::StandardContentComponent (Context& ctl_)
    : ContentComponent (ctl_)
{
    setOpaque (true);

    addAndMakeVisible (container = new ContentContainer (*this, services()));
    addAndMakeVisible (bar1 = new Resizer (*this, &layout, 1, true));
    addAndMakeVisible (nav = new NavigationConcertinaPanel (ctl_));
    nav->updateContent();

    toolBarVisible = true;
    toolBarSize = 32;
    statusBarVisible = true;
    statusBarSize = 22;

    setSize (640, 360);

    container->setMainView (new GraphEditorView());

    nav->setSize (304, getHeight());
    resizerMouseUp();
    if (auto pp = nav->findPanel<PluginsPanelView>())
        nav->setPanelSize (pp, 20 * 4, false);
    resized();

    setVirtualKeyboardVisible (true);
    setNodeChannelStripVisible (false);
    setMeterBridgeVisible (false);
}

StandardContentComponent::~StandardContentComponent() noexcept
{
    setContentView (nullptr, false);
    setContentView (nullptr, true);
}

String StandardContentComponent::getMainViewName() const
{
    String name;
    if (auto c1 = container->primary.get())
        name = c1->getName();
    return name;
}

String StandardContentComponent::getAccessoryViewName() const
{
    String name;
    if (auto c2 = container->secondary.get())
        name = c2->getName();
    return name;
}

int StandardContentComponent::getNavSize()
{
    return nav != nullptr ? nav->getWidth() : 220;
}

void StandardContentComponent::setMainView (const String& name)
{
    if (name == "PatchBay")
    {
        setContentView (new ConnectionGrid());
    }
    else if (name == "GraphEditor" || name == "GraphEditorView")
    {
        setContentView (createGraphEditorView());
    }
    else if (name == "PluginManager")
    {
        setContentView (new PluginManagerContentView());
    }
    else if (name == "SessionSettings" || name == "SessionProperties")
    {
        setContentView (new SessionContentView());
    }
    else if (name == "GraphSettings")
    {
        setContentView (new GraphSettingsView());
    }
    else if (name == "KeymapEditorView")
    {
        setContentView (new KeymapEditorView());
    }
    else if (name == "ControllersView")
    {
        setContentView (new ControllersView());
    }
    else
    {
        if (auto s = context().session())
        {
            if (s->getNumGraphs() > 0)
                setContentView (createGraphEditorView());
            else
                setContentView (new EmptyContentView());
        }
        else
        {
            setContentView (new EmptyContentView());
        }
    }
}

void StandardContentComponent::backMainView()
{
    setMainView (lastMainView.isEmpty() ? "GraphEditor" : lastMainView);
}

void StandardContentComponent::nextMainView()
{
    // only have two rotatable views as of now
    if (getMainViewName() == "EmptyView")
        return;
    const String nextName = getMainViewName() == "GraphEditor" ? "PatchBay" : "GraphEditor";
    setMainView (nextName);
}

void StandardContentComponent::setContentView (ContentView* view, const bool accessory)
{
    std::unique_ptr<ContentView> deleter (view);
    if (accessory)
    {
        container->setSecondaryView (deleter.release());
    }
    else
    {
        lastMainView = getMainViewName();
        container->setMainView (deleter.release());
    }
}

void StandardContentComponent::setSecondaryView (const String& name)
{
    if (name == EL_VIEW_GRAPH_MIXER)
    {
        setContentView (new GraphMixerView(), true);
    }
    else if (name == EL_VIEW_CONSOLE)
    {
        setContentView (new LuaConsoleView(), true);
    }
}

void StandardContentComponent::resizeContent (const Rectangle<int>& area)
{
    Rectangle<int> r (area);

    if (nodeStrip && nodeStrip->isVisible())
        nodeStrip->setBounds (r.removeFromRight (nodeStripSize));

    Component* comps[3] = { nav.get(), bar1.get(), container.get() };
    layout.layOutComponents (comps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);
}

bool StandardContentComponent::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    const auto& desc (dragSourceDetails.description);
    return desc.toString() == "ccNavConcertinaPanel" || (desc.isArray() && desc.size() >= 2 && desc[0] == "plugin");
}

void StandardContentComponent::itemDropped (const SourceDetails& dragSourceDetails)
{
    const auto& desc (dragSourceDetails.description);
    if (desc.toString() == "ccNavConcertinaPanel")
    {
        if (auto* panel = nav->findPanel<DataPathTreeComponent>())
            filesDropped (StringArray ({ panel->getSelectedFile().getFullPathName() }),
                          dragSourceDetails.localPosition.getX(),
                          dragSourceDetails.localPosition.getY());
    }
    else if (desc.isArray() && desc.size() >= 2 && desc[0] == "plugin")
    {
        auto& list (context().plugins().getKnownPlugins());
        if (auto plugin = list.getTypeForIdentifierString (desc[1].toString()))
            this->post (new LoadPluginMessage (*plugin, true));
        else
            AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon, "Could not load plugin", "The plugin you dropped could not be loaded for an unknown reason.");
    }
}

bool StandardContentComponent::isInterestedInFileDrag (const StringArray& files)
{
    for (const auto& path : files)
    {
        const File file (path);
        if (file.hasFileExtension ("elc;elg;els;dll;vst3;vst;elpreset"))
            return true;
    }
    return false;
}

void StandardContentComponent::filesDropped (const StringArray& files, int x, int y)
{
    for (const auto& path : files)
    {
        const File file (path);
        if (file.hasFileExtension ("els"))
        {
            this->post (new OpenSessionMessage (file));
        }
        else if (file.hasFileExtension ("elg"))
        {
            if (auto* sess = services().find<SessionService>())
                sess->importGraph (file);
        }
        else if (file.hasFileExtension ("elpreset"))
        {
            const auto data = Node::parse (file);
            if (data.hasType (types::Node))
            {
                const Node node (data, false);
                this->post (new AddNodeMessage (node));
            }
            else
            {
                AlertWindow::showMessageBox (AlertWindow::InfoIcon, "Presets", "Error adding preset");
            }
        }
        else if ((file.hasFileExtension ("dll") || file.hasFileExtension ("vst") || file.hasFileExtension ("vst3")) && (getMainViewName() == "GraphEditor" || getMainViewName() == "PatchBay" || getMainViewName() == "PluginManager"))
        {
            auto s = session();
            auto graph = s->getActiveGraph();
            PluginDescription desc;
            desc.pluginFormatName = file.hasFileExtension ("vst3") ? "VST3" : "VST";
            desc.fileOrIdentifier = file.getFullPathName();

            auto message = std::make_unique<AddPluginMessage> (graph, desc, false);
            auto& builder (message->builder);

            if (ModifierKeys::getCurrentModifiersRealtime().isAltDown())
            {
                const auto audioInputNode = graph.getIONode (PortType::Audio, true);
                const auto midiInputNode = graph.getIONode (PortType::Midi, true);
                builder.addChannel (audioInputNode, PortType::Audio, 0, 0, false);
                builder.addChannel (audioInputNode, PortType::Audio, 1, 1, false);
                builder.addChannel (midiInputNode, PortType::Midi, 0, 0, false);
            }

            if (ModifierKeys::getCurrentModifiersRealtime().isCommandDown())
            {
                const auto audioOutputNode = graph.getIONode (PortType::Audio, false);
                const auto midiOutNode = graph.getIONode (PortType::Midi, false);
                builder.addChannel (audioOutputNode, PortType::Audio, 0, 0, true);
                builder.addChannel (audioOutputNode, PortType::Audio, 1, 1, true);
                builder.addChannel (midiOutNode, PortType::Midi, 0, 0, true);
            }

            this->post (message.release());
        }
    }
}

void StandardContentComponent::stabilize (const bool refreshDataPathTrees)
{
    auto session = context().session();
    if (session->getNumGraphs() > 0)
    {
        const Node graph = session->getCurrentGraph();
        setCurrentNode (graph);
    }
    else
    {
        setContentView (new EmptyContentView());
    }

    if (auto* ss = nav->findPanel<SessionTreePanel>())
        ss->setSession (session);
    if (auto* mcv = nav->findPanel<NodeMidiContentView>())
        mcv->stabilizeContent();
    if (auto* ncv = nav->findPanel<NodeEditorContentView>())
        ncv->stabilizeContent();
    if (auto* gcv = nav->findPanel<GraphSettingsView>())
        gcv->stabilizeContent();

    stabilizeViews();

    if (auto* main = findParentComponentOfClass<MainWindow>())
        main->refreshMenu();

    if (refreshDataPathTrees)
        if (auto* data = nav->findPanel<DataPathTreeComponent>())
            data->refresh();

    refreshToolbar();
    refreshStatusBar();
}

void StandardContentComponent::stabilizeViews()
{
    if (container->primary)
        container->primary->stabilizeContent();
    if (auto c2 = container->secondary.get())
        c2->stabilizeContent();
    if (nodeStrip)
        nodeStrip->stabilizeContent();
}

void StandardContentComponent::saveState (PropertiesFile* props)
{
    jassert (props);
    if (nav)
        nav->saveState (props);
    if (container)
        container->saveState (props);
    if (auto* const vk = getVirtualKeyboardView())
    {
        vk->saveState (props);
        props->setValue ("virtualKeyboard", isVirtualKeyboardVisible());
    }

    auto& mo = container->bottom->bridge->getMeterBridge();
    props->setValue ("meterBridge", isMeterBridgeVisible());
    props->setValue ("meterBridgeSize", mo.getMeterSize());
    props->setValue ("meterBridgeVisibility", (int) mo.visibility());
}

void StandardContentComponent::restoreState (PropertiesFile* props)
{
    jassert (props);
    if (nav)
        nav->restoreState (props);
    if (container)
        container->restoreState (props);
    if (auto* const vk = getVirtualKeyboardView())
    {
        vk->restoreState (props);
        setVirtualKeyboardVisible (props->getBoolValue ("virtualKeyboard", isVirtualKeyboardVisible()));
    }

    auto& bo = container->bottom->bridge->getMeterBridge();
    bo.setMeterSize (props->getIntValue ("meterBridgeSize", bo.getMeterSize()));
    bo.setVisibility ((uint32) props->getIntValue ("meterBridgeVisibility", bo.visibility()));
    setMeterBridgeVisible (props->getBoolValue ("meterBridge", isMeterBridgeVisible()));
    resized();
}

void StandardContentComponent::setCurrentNode (const Node& node)
{
    if ((nullptr != dynamic_cast<EmptyContentView*> (container->primary.get()) || getMainViewName() == "SessionSettings" || getMainViewName() == "PluginManager" || getMainViewName() == "ControllersView") && session()->getNumGraphs() > 0)
    {
        setMainView ("GraphEditor");
    }

    container->setNode (node);
}

void StandardContentComponent::updateLayout()
{
    layout.setItemLayout (0, EL_NAV_MIN_WIDTH, EL_NAV_MAX_WIDTH, nav->getWidth());
    layout.setItemLayout (1, 2, 2, 2);
    layout.setItemLayout (2, 100, -1, 400);
}

void StandardContentComponent::resizerMouseDown()
{
    updateLayout();
}

void StandardContentComponent::resizerMouseUp()
{
    layout.setItemLayout (0, nav->getWidth(), nav->getWidth(), nav->getWidth());
    layout.setItemLayout (1, 2, 2, 2);
    layout.setItemLayout (2, 100, -1, 400);
    resized();
}

void StandardContentComponent::setNodeChannelStripVisible (const bool isVisible)
{
    if (! nodeStrip)
    {
        nodeStrip = new NodeChannelStripView();
        nodeStrip->initializeView (services());
    }

    if (isVisible == nodeStrip->isVisible())
        return;

    if (isVisible)
    {
        nodeStrip->willBecomeActive();
        addAndMakeVisible (nodeStrip);
        nodeStrip->didBecomeActive();
        nodeStrip->stabilizeContent();
        if (nodeStrip->isShowing() || nodeStrip->isOnDesktop())
            nodeStrip->grabKeyboardFocus();
    }
    else
    {
        nodeStrip->setVisible (false);
    }

    resized();
}
bool StandardContentComponent::isNodeChannelStripVisible() const { return nodeStrip && nodeStrip->isVisible(); }

//=====
bool StandardContentComponent::isVirtualKeyboardVisible() const
{
    if (auto vc = getVirtualKeyboardView())
        return vc->isVisible();
    return false;
}

void StandardContentComponent::setVirtualKeyboardVisible (const bool vis)
{
    auto keyboard = getVirtualKeyboardView();
    if (keyboard->isVisible() == vis)
        return;

    keyboard->setVisible (vis);
    if (isVirtualKeyboardVisible())
    {
        if (keyboard->isShowing() || keyboard->isOnDesktop())
            keyboard->grabKeyboardFocus();
    }
    else
    {
        keyboard->setVisible (false);
    }

    container->resized();
}

void StandardContentComponent::toggleVirtualKeyboard()
{
    setVirtualKeyboardVisible (! isVirtualKeyboardVisible());
}

VirtualKeyboardView* StandardContentComponent::getVirtualKeyboardView() const
{
    return container->bottom->keyboard.get();
}

//==============================================================================
ApplicationCommandTarget* StandardContentComponent::getNextCommandTarget()
{
    return nullptr;
}

void StandardContentComponent::getAllCommands (Array<CommandID>& commands)
{
    // clang-format off
    commands.addArray ({
        Commands::showControllers,
        Commands::showKeymapEditor,
        Commands::showPluginManager,
        Commands::showSessionConfig,
        Commands::showGraphConfig,
        Commands::showPatchBay,
        Commands::showGraphEditor,
        Commands::showGraphMixer,
        Commands::showConsole,
        Commands::toggleVirtualKeyboard,
        Commands::toggleMeterBridge,
        Commands::toggleChannelStrip,
        Commands::showLastContentView,
        Commands::rotateContentView
    });
    // clang-format on
}

void StandardContentComponent::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    using Info = ApplicationCommandInfo;

    switch (commandID)
    {
        case Commands::showControllers: {
            int flags = 0;
            if (getMainViewName() == "ControllersView")
                flags |= Info::isTicked;
            result.setInfo ("Controllers", "Show the session's controllers", "UI", flags);
            break;
        }
        case Commands::showKeymapEditor: {
            int flags = 0;
            // FIXME: wrong view name.
            if (getMainViewName() == "KeymapView")
                flags |= Info::isTicked;
            result.setInfo ("Keymappings", "Show the session's controllers", "UI", flags);
            break;
        }
        case Commands::showPluginManager: {
            int flags = 0;
            // FIXME: wrong view name.
            if (getMainViewName() == "PluginManagerView")
                flags |= Info::isTicked;
            result.setInfo ("Plugin Manager", "Show the session's controllers", "UI", flags);
            break;
        }
        //=====
        case Commands::showSessionConfig: {
            int flags = 0;
            if (getMainViewName() == "SessionSettings")
                flags |= Info::isTicked;
            result.setInfo ("Session Settings", "Session Settings", "Session", flags);
        }
        //=====
        case Commands::showGraphConfig: {
            int flags = 0;
            if (getMainViewName() == "GraphSettings")
                flags |= Info::isTicked;
            result.setInfo ("Graph Settings", "Graph Settings", "Graph", flags);
            break;
        }
        //===
        case Commands::showPatchBay: {
            int flags = 0;
            if (getMainViewName() == "PatchBay")
                flags |= Info::isTicked;
            result.addDefaultKeypress (KeyPress::F1Key, 0);
            result.setInfo ("Patch Bay", "Show the patch bay", "UI", flags);
            break;
        }

        case Commands::showGraphEditor: {
            int flags = 0;
            if (getMainViewName() == "GraphEditor")
                flags |= Info::isTicked;
            result.addDefaultKeypress (KeyPress::F2Key, 0);
            result.setInfo ("Graph Editor", "Show the graph editor", "UI", flags);
        }
        break;
            //===
        case Commands::showGraphMixer: {
            int flags = (showAccessoryView() && getAccessoryViewName() == EL_VIEW_GRAPH_MIXER)
                            ? Info::isTicked
                            : 0;
            result.setInfo ("Graph Mixer", "Show/hide the graph mixer", "UI", flags);
            break;
        }
        //======================================================================
        case Commands::showConsole: {
            int flags = (showAccessoryView() && getAccessoryViewName() == EL_VIEW_CONSOLE)
                            ? Info::isTicked
                            : 0;
            result.setInfo ("Console", "Show the scripting console", "UI", flags);
            break;
        }
        //======================================================================
        case Commands::toggleVirtualKeyboard: {
            int flags = 0;
            if (isVirtualKeyboardVisible())
                flags |= Info::isTicked;
            result.setInfo ("Virtual Keyboard", "Toggle the virtual keyboard", "UI", flags);
            break;
        }
        case Commands::toggleMeterBridge: {
            int flags = 0;
            if (isMeterBridgeVisible())
                flags |= Info::isTicked;
            result.setInfo ("MeterBridge", "Toggle the Meter Bridge", "UI", flags);
            break;
        }
        case Commands::toggleChannelStrip: {
            int flags = 0;
            if (isNodeChannelStripVisible())
                flags |= Info::isTicked;
            result.setInfo ("Channel Strip", "Toggles the global channel strip", "UI", flags);
            break;
        }
        case Commands::showLastContentView: {
            result.setInfo ("Last View", "Shows the last shown View", "UI", 0);
            break;
        }
        case Commands::rotateContentView: {
            result.setInfo ("Next View", "Shows the next View", "UI", 0);
            break;
        }
    }
}

bool StandardContentComponent::perform (const InvocationInfo& info)
{
    bool result = true;
    switch (info.commandID)
    {
        case Commands::showControllers: {
            setMainView ("ControllersView");
            break;
        }
        case Commands::showKeymapEditor:
            setMainView ("KeymapEditorView");
            break;
        case Commands::showPluginManager:
            setMainView ("PluginManager");
            break;
            //===
        case Commands::showSessionConfig:
            setMainView ("SessionSettings");
            break;
        case Commands::showGraphConfig:
            setMainView ("GraphSettings");
            break;
            //===

        case Commands::showPatchBay:
            setMainView ("PatchBay");
            break;
        case Commands::showGraphEditor:
            setMainView ("GraphEditor");
            break;
        //======================================================================
        case Commands::showGraphMixer: {
            if (showAccessoryView() && getAccessoryViewName() == EL_VIEW_GRAPH_MIXER)
            {
                setShowAccessoryView (false);
            }
            else
            {
                setSecondaryView (EL_VIEW_GRAPH_MIXER);
            }
            break;
        }
        case Commands::showConsole: {
            if (showAccessoryView() && getAccessoryViewName() == EL_VIEW_CONSOLE)
            {
                setShowAccessoryView (false);
            }
            else
            {
                setSecondaryView (EL_VIEW_CONSOLE);
            }
            break;
        }

        case Commands::toggleVirtualKeyboard:
            toggleVirtualKeyboard();
            break;
        case Commands::toggleMeterBridge:
            setMeterBridgeVisible (! isMeterBridgeVisible());
            break;
        case Commands::toggleChannelStrip:
            setNodeChannelStripVisible (! isNodeChannelStripVisible());
            break;
        case Commands::showLastContentView:
            backMainView();
            break;
        case Commands::rotateContentView:
            nextMainView();
            break;
        default:
            result = false;
            break;
    }

    if (result)
    {
        services().find<UI>()->refreshMainMenu();
    }
    return result;
}

void StandardContentComponent::setShowAccessoryView (const bool show)
{
    if (container)
        container->setShowAccessoryView (show);
}

bool StandardContentComponent::showAccessoryView() const
{
    return (container) ? container->showAccessoryView : false;
}

void StandardContentComponent::getSessionState (String& state)
{
    ValueTree data ("state");

    if (auto* const ned = nav->findPanel<NodeEditorContentView>())
    {
        String nedState;
        ned->getState (nedState);
        if (nedState.isNotEmpty())
        {
            data.setProperty ("NodeEditorContentView", nedState, nullptr);
        }
    }

    MemoryOutputStream mo;
    {
        GZIPCompressorOutputStream gzip (mo, 9);
        data.writeToStream (gzip);
    }

    state = mo.getMemoryBlock().toBase64Encoding();
}

void StandardContentComponent::applySessionState (const String& state)
{
    MemoryBlock mb;
    mb.fromBase64Encoding (state);
    const ValueTree data = (mb.getSize() > 0)
                               ? ValueTree::readFromGZIPData (mb.getData(), mb.getSize())
                               : ValueTree();
    if (! data.isValid())
        return;

    if (auto* const ned = nav->findPanel<NodeEditorContentView>())
    {
        String nedState = data.getProperty ("NodeEditorContentView").toString();
        ned->setState (nedState);
    }
}

void StandardContentComponent::presentView (std::unique_ptr<ContentView> view)
{
    setContentView (view.release(), false);
}

void StandardContentComponent::setMainView (ContentView* view)
{
    jassert (view != nullptr);
    setContentView (view, false);
}

void StandardContentComponent::setMeterBridgeVisible (bool vis)
{
    if (isMeterBridgeVisible() == vis)
        return;
    container->bottom->bridge->setVisible (vis);
    container->resized();
    resized();
}

bool StandardContentComponent::isMeterBridgeVisible() const
{
    return container->bottom->bridge->isVisible();
}

} // namespace element
