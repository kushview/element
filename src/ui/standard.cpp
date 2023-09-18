// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/services.hpp>
#include <element/ui/meterbridge.hpp>
#include "ui/datapathbrowser.hpp"

#include "services/mappingservice.hpp"
#include "services/sessionservice.hpp"
#include "ui/emptyview.hpp"
#include "ui/audioiopanelview.hpp"
#include "ui/pluginspanelview.hpp"
#include "ui/connectiongrid.hpp"
#include "ui/controllersview.hpp"
#include "ui/grapheditorview.hpp"
#include "ui/graphmixerview.hpp"
#include "ui/keymapeditorview.hpp"
#include "ui/luaconsoleview.hpp"
#include "ui/nodechannelstripview.hpp"
#include <element/ui/mainwindow.hpp>
#include "ui/mainmenu.hpp"
#include "ui/midiblinker.hpp"
#include "ui/navigationview.hpp"
#include "ui/sessiontreepanel.hpp"
#include "ui/viewhelpers.hpp"
#include <element/ui/style.hpp>
#include "ui/pluginmanagercomponent.hpp"
#include "ui/sessionsettingsview.hpp"
#include "ui/graphsettingsview.hpp"
#include "ui/virtualkeyboardview.hpp"
#include "ui/tempoandmeterbar.hpp"
#include "ui/transportbar.hpp"
#include <element/ui/navigation.hpp>
#include "ui/nodeeditorview.hpp"
#include "ui/nodepropertiesview.hpp"
#include "ui/pluginspanelview.hpp"
#include "ui/audioiopanelview.hpp"
#include "ui/sessiontreepanel.hpp"

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
    ContentContainer (StandardContent& cc, Services& app)
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

        setShowAccessoryView (secondary != nullptr, true);

        if (secondary)
        {
            secondary->didBecomeActive();
            secondary->stabilizeContent();
        }
    }

    void setShowAccessoryView (const bool show, bool force = false)
    {
        if (showAccessoryView == show && ! force)
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
        props->setValue ("ContentContainer_showAccessoryView", showAccessoryView);
        props->setValue ("ContentContainer_lastSecondaryView", owner.getAccessoryViewName());
    }

    void restoreState (PropertiesFile* props)
    {
        lastSecondaryHeight = props->getIntValue ("ContentContainer_lastSecondaryHeight", lastSecondaryHeight);
        lastSecondaryHeight = jmax (50, lastSecondaryHeight);
        showAccessoryView = props->getIntValue ("ContentContainer_showAccessoryView", showAccessoryView);
        auto lastSecondaryName = props->getValue ("ContentContainer_lastSecondaryView");
        if (showAccessoryView)
        {
            owner.setSecondaryView (lastSecondaryName.trim());
        }
        else
        {
            setShowAccessoryView (false, true);
        }
    }

private:
    friend class StandardContent;
    StandardContent& owner;

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

class StandardContent::Resizer : public StretchableLayoutResizerBar
{
public:
    Resizer (StandardContent& StandardContent, StretchableLayoutManager* layoutToUse, int itemIndexInLayout, bool isBarVertical)
        : StretchableLayoutResizerBar (layoutToUse, itemIndexInLayout, isBarVertical),
          owner (StandardContent)
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
    StandardContent& owner;
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
    else if (lastContentView == EL_VIEW_GRAPH_EDITOR)
        view.reset (new GraphEditorView());
    else if (lastContentView == EL_VIEW_CONTROLLERS)
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
StandardContent::StandardContent (Context& ctl_)
    : Content (ctl_)
{
    setOpaque (true);

    container = std::make_unique<ContentContainer> (*this, services());
    addAndMakeVisible (container.get());
    bar1 = std::make_unique<Resizer> (*this, &layout, 1, true);
    addAndMakeVisible (bar1.get());
    nav = std::make_unique<NavigationConcertinaPanel> (ctl_);
    addAndMakeVisible (nav.get());
    nav->updateContent();

    toolBarVisible = true;
    toolBarSize = 32;
    statusBarVisible = true;
    statusBarSize = 22;

    setSize (640, 360);

    setMainView (EL_VIEW_GRAPH_EDITOR);

    nav->setSize (304, getHeight());
    resizerMouseUp();
    if (auto pp = nav->findPanel<PluginsPanelView>())
        nav->setPanelSize (pp, 20 * 4, false);
    resized();

    setVirtualKeyboardVisible (true);
    setNodeChannelStripVisible (false);
    setMeterBridgeVisible (false);
}

StandardContent::~StandardContent() noexcept
{
    setContentView (nullptr, false);
    setContentView (nullptr, true);
}

String StandardContent::getMainViewName() const
{
    String name;
    if (auto c1 = container->primary.get())
        name = c1->getName();
    return name;
}

Component* StandardContent::getMainViewComponent() const
{
    if (auto c1 = container->primary.get())
        return c1;
    return nullptr;
}

String StandardContent::getAccessoryViewName() const
{
    String name;
    if (auto c2 = container->secondary.get())
        name = c2->getName();
    return name;
}

int StandardContent::getNavSize()
{
    return nav != nullptr ? nav->getWidth() : 220;
}

void StandardContent::setMainView (const String& name)
{
    if (auto v = createContentView (name))
    {
        v->setName (name);
        setContentView (v, false);
        return;
    }

    if (name == "PatchBay")
    {
        setContentView (new ConnectionGrid());
    }
    else if (name == EL_VIEW_GRAPH_EDITOR)
    {
        setContentView (new GraphEditorView());
    }
    else if (name == EL_VIEW_PLUGIN_MANAGER)
    {
        setContentView (new PluginManagerContentView());
    }
    else if (name == EL_VIEW_SESSION_SETTINGS || name == "SessionProperties")
    {
        setContentView (new SessionContentView());
    }
    else if (name == "GraphSettings")
    {
        setContentView (new GraphSettingsView());
    }
    else if (name == EL_VIEW_KEYMAP_EDITOR)
    {
        setContentView (new KeymapEditorView());
    }
    else if (name == EL_VIEW_CONTROLLERS)
    {
        setContentView (new ControllersView());
    }
    else
    {
        if (auto s = context().session())
        {
            if (s->getNumGraphs() > 0)
                setContentView (new GraphEditorView());
            else
                setContentView (new EmptyContentView());
        }
        else
        {
            setContentView (new EmptyContentView());
        }
    }
}

void StandardContent::backMainView()
{
    setMainView (lastMainView.isEmpty() ? EL_VIEW_GRAPH_EDITOR : lastMainView);
}

void StandardContent::nextMainView()
{
    // only have two rotatable views as of now
    if (getMainViewName() == "EmptyView")
        return;
    const String nextName = getMainViewName() == EL_VIEW_GRAPH_EDITOR ? "PatchBay" : EL_VIEW_GRAPH_EDITOR;
    setMainView (nextName);
}

void StandardContent::setContentView (ContentView* view, const bool accessory)
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

void StandardContent::setSecondaryView (const String& name)
{
    if (auto v = createContentView (name))
    {
        v->setName (name);
        setContentView (v, true);
        return;
    }

    if (name == EL_VIEW_GRAPH_MIXER)
    {
        setContentView (new GraphMixerView(), true);
    }
    else if (name == EL_VIEW_CONSOLE)
    {
        setContentView (new LuaConsoleView(), true);
    }
}

void StandardContent::resizeContent (const Rectangle<int>& area)
{
    Rectangle<int> r (area);

    if (_extra && _extra->isVisible())
    {
        _extra->setBounds (r.removeFromBottom (44));
        r.removeFromBottom (1);
    }

    if (nodeStrip && nodeStrip->isVisible())
        nodeStrip->setBounds (r.removeFromRight (nodeStripSize));

    Component* comps[3] = { nav.get(), bar1.get(), container.get() };
    layout.layOutComponents (comps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);
}

bool StandardContent::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    const auto& desc (dragSourceDetails.description);
    return desc.toString() == "ccNavConcertinaPanel" || (desc.isArray() && desc.size() >= 2 && desc[0] == "plugin");
}

void StandardContent::itemDropped (const SourceDetails& dragSourceDetails)
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

bool StandardContent::isInterestedInFileDrag (const StringArray& files)
{
    for (const auto& path : files)
    {
        const File file (path);
        if (file.hasFileExtension ("elc;elg;els;dll;vst3;vst;elpreset"))
            return true;
    }
    return false;
}

void StandardContent::filesDropped (const StringArray& files, int x, int y)
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
        else if ((file.hasFileExtension ("dll") || file.hasFileExtension ("vst") || file.hasFileExtension ("vst3")) && (getMainViewName() == EL_VIEW_GRAPH_EDITOR || getMainViewName() == "PatchBay" || getMainViewName() == EL_VIEW_PLUGIN_MANAGER))
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

void StandardContent::stabilize (const bool refreshDataPathTrees)
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
    if (auto* mcv = nav->findPanel<NodePropertiesView>())
        mcv->stabilizeContent();
    if (auto* ncv = nav->findPanel<NodeEditorView>())
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

void StandardContent::stabilizeViews()
{
    if (container->primary)
        container->primary->stabilizeContent();
    if (auto c2 = container->secondary.get())
        c2->stabilizeContent();
    if (nodeStrip)
        nodeStrip->stabilizeContent();
}

void StandardContent::saveState (PropertiesFile* props)
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

    props->setValue ("channelStrip", isNodeChannelStripVisible());

    auto& mo = container->bottom->bridge->meterBridge();
    props->setValue ("meterBridge", isMeterBridgeVisible());
    props->setValue ("meterBridgeSize", mo.meterSize());
    props->setValue ("meterBridgeVisibility", (int) mo.visibility());
}

void StandardContent::restoreState (PropertiesFile* props)
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

    setNodeChannelStripVisible (props->getBoolValue ("channelStrip", isNodeChannelStripVisible()));

    auto& bo = container->bottom->bridge->meterBridge();
    bo.setMeterSize (props->getIntValue ("meterBridgeSize", bo.meterSize()));
    bo.setVisibility ((uint32) props->getIntValue ("meterBridgeVisibility", bo.visibility()));
    setMeterBridgeVisible (props->getBoolValue ("meterBridge", isMeterBridgeVisible()));

    resized();
    container->bottom->resized();
    container->resized();
}

void StandardContent::setCurrentNode (const Node& node)
{
    // clang-format off
    if ((nullptr != dynamic_cast<EmptyContentView*> (container->primary.get()) || 
        getMainViewName() == EL_VIEW_SESSION_SETTINGS || 
        getMainViewName() == EL_VIEW_PLUGIN_MANAGER || 
        getMainViewName() == EL_VIEW_CONTROLLERS) && 
        session()->getNumGraphs() > 0)
    {
        setMainView (EL_VIEW_GRAPH_EDITOR);
    }
    // clang-format on

    container->setNode (node);
}

void StandardContent::updateLayout()
{
    layout.setItemLayout (0, EL_NAV_MIN_WIDTH, EL_NAV_MAX_WIDTH, nav->getWidth());
    layout.setItemLayout (1, 2, 2, 2);
    layout.setItemLayout (2, 100, -1, 400);
}

void StandardContent::resizerMouseDown()
{
    updateLayout();
}

void StandardContent::resizerMouseUp()
{
    layout.setItemLayout (0, nav->getWidth(), nav->getWidth(), nav->getWidth());
    layout.setItemLayout (1, 2, 2, 2);
    layout.setItemLayout (2, 100, -1, 400);
    resized();
}

void StandardContent::setNodeChannelStripVisible (const bool isVisible)
{
    if (! nodeStrip)
    {
        nodeStrip = std::make_unique<NodeChannelStripView>();
        nodeStrip->initializeView (services());
    }

    if (isVisible == nodeStrip->isVisible())
        return;

    if (isVisible)
    {
        nodeStrip->willBecomeActive();
        addAndMakeVisible (nodeStrip.get());
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
    container->bottom->resized();
    container->resized();
}
bool StandardContent::isNodeChannelStripVisible() const { return nodeStrip && nodeStrip->isVisible(); }

//==============================================================================
bool StandardContent::isVirtualKeyboardVisible() const
{
    if (auto vc = getVirtualKeyboardView())
        return vc->isVisible();
    return false;
}

void StandardContent::setVirtualKeyboardVisible (const bool vis)
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

void StandardContent::toggleVirtualKeyboard()
{
    setVirtualKeyboardVisible (! isVirtualKeyboardVisible());
}

VirtualKeyboardView* StandardContent::getVirtualKeyboardView() const
{
    return container->bottom->keyboard.get();
}

//==============================================================================
ApplicationCommandTarget* StandardContent::getNextCommandTarget()
{
    return nullptr;
}

void StandardContent::getAllCommands (Array<CommandID>& commands)
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
        Commands::rotateContentView,
        Commands::selectAll
    });
    // clang-format on
}

void StandardContent::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    using Info = ApplicationCommandInfo;

    switch (commandID)
    {
        case Commands::showControllers: {
            int flags = 0;
            if (getMainViewName() == EL_VIEW_CONTROLLERS)
                flags |= Info::isTicked;
            result.setInfo ("Controllers", "Show the session's controllers", "UI", flags);
            break;
        }
        case Commands::showKeymapEditor: {
            int flags = 0;
            if (getMainViewName() == EL_VIEW_KEYMAP_EDITOR)
                flags |= Info::isTicked;
            result.setInfo ("Keymappings", "Show the session's controllers", "UI", flags);
            break;
        }
        case Commands::showPluginManager: {
            int flags = 0;
            if (getMainViewName() == EL_VIEW_PLUGIN_MANAGER)
                flags |= Info::isTicked;
            result.setInfo ("Plugin Manager", "Show the session's controllers", "UI", flags);
            break;
        }
        //=====
        case Commands::showSessionConfig: {
            int flags = 0;
            if (getMainViewName() == EL_VIEW_SESSION_SETTINGS)
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
            if (getMainViewName() == EL_VIEW_GRAPH_EDITOR)
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
        case Commands::selectAll: {
            int flags = 0;
            result.setInfo ("Select all", "Select all nodes", "UI", flags);
            result.addDefaultKeypress ('a', ModifierKeys::commandModifier);
            break;
        }
    }
}

bool StandardContent::perform (const InvocationInfo& info)
{
    bool result = true;
    switch (info.commandID)
    {
        case Commands::showControllers: {
            setMainView (EL_VIEW_CONTROLLERS);
            break;
        }
        case Commands::showKeymapEditor:
            setMainView (EL_VIEW_KEYMAP_EDITOR);
            break;
        case Commands::showPluginManager:
            setMainView (EL_VIEW_PLUGIN_MANAGER);
            break;
            //===
        case Commands::showSessionConfig:
            setMainView (EL_VIEW_SESSION_SETTINGS);
            break;
        case Commands::showGraphConfig:
            setMainView ("GraphSettings");
            break;
            //===

        case Commands::showPatchBay:
            setMainView ("PatchBay");
            break;
        case Commands::showGraphEditor:
            setMainView (EL_VIEW_GRAPH_EDITOR);
            break;
        //======================================================================
        case Commands::showGraphMixer: {
            if (showAccessoryView() && getAccessoryViewName() == EL_VIEW_GRAPH_MIXER)
            {
                container->setSecondaryView (nullptr);
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
                container->setSecondaryView (nullptr);
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
        case Commands::selectAll: {
            if (auto view = dynamic_cast<GraphEditorView*> (container->primary.get()))
                view->selectAllNodes();
            break;
        }
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

void StandardContent::setShowAccessoryView (const bool show)
{
    if (container)
        container->setShowAccessoryView (show);
}

bool StandardContent::showAccessoryView() const
{
    return (container) ? container->showAccessoryView : false;
}

void StandardContent::getSessionState (String& state)
{
    ValueTree data ("state");

    if (auto* const ned = nav->findPanel<NodeEditorView>())
    {
        String nedState;
        ned->getState (nedState);
        if (nedState.isNotEmpty())
        {
            data.setProperty ("NodeEditorView", nedState, nullptr);
        }
    }

    if (auto* const npv = nav->findPanel<NodePropertiesView>())
    {
        String npvState;
        npv->getState (npvState);
        if (npvState.isNotEmpty())
        {
            data.setProperty ("NodePropertiesView", npvState, nullptr);
        }
    }

    MemoryOutputStream mo;
    {
        GZIPCompressorOutputStream gzip (mo, 9);
        data.writeToStream (gzip);
    }

    state = mo.getMemoryBlock().toBase64Encoding();
}

void StandardContent::applySessionState (const String& state)
{
    MemoryBlock mb;
    mb.fromBase64Encoding (state);
    const ValueTree data = (mb.getSize() > 0)
                               ? ValueTree::readFromGZIPData (mb.getData(), mb.getSize())
                               : ValueTree();
    if (! data.isValid())
        return;

    if (auto* const ned = nav->findPanel<NodeEditorView>())
    {
        String nedState = data.getProperty ("NodeEditorView").toString();
        ned->setState (nedState);
    }

    if (auto* const npv = nav->findPanel<NodePropertiesView>())
    {
        String npvState = data.getProperty ("NodePropertiesView").toString();
        npv->setState (npvState);
    }
}

void StandardContent::presentView (const juce::String& view)
{
    setMainView (view);
}

void StandardContent::presentView (std::unique_ptr<View> view)
{
    if (view == nullptr)
        return;

    class ViewWrapper : public ContentView
    {
    public:
        ViewWrapper()
        {
            setSize (1, 1);
            setInterceptsMouseClicks (false, true);
        }

        ~ViewWrapper() { view.reset(); }

        void init()
        {
            addAndMakeVisible (view.get());
        }

        void paint (juce::Graphics& g) override
        {
            if (isOpaque())
                g.fillAll (Colours::black);
        }

        void resized() override
        {
            if (view)
                view->setBounds (getLocalBounds());
        }

        std::unique_ptr<View> view;
    };

    if (auto c = dynamic_cast<ContentView*> (view.get()))
    {
        view.release();
        setContentView (c, false);
    }
    else
    {
        auto wrap = new ViewWrapper();
        wrap->view = std::move (view);
        wrap->setOpaque (wrap->view->isOpaque());
        wrap->addAndMakeVisible (wrap->view.get());
        wrap->resized();
        setContentView (wrap, false);
    }
}

void StandardContent::setMainView (ContentView* view)
{
    jassert (view != nullptr);
    setContentView (view, false);
}

void StandardContent::setExtraView (Component* extra)
{
    _extra.reset (extra);
    if (_extra)
    {
        addAndMakeVisible (_extra.get());
    }
    resized();
}

void StandardContent::setMeterBridgeVisible (bool vis)
{
    if (isMeterBridgeVisible() == vis)
        return;
    container->bottom->bridge->setVisible (vis);
    container->bottom->resized();
    container->resized();
}

bool StandardContent::isMeterBridgeVisible() const
{
    return container->bottom->bridge->isVisible();
}

} // namespace element
