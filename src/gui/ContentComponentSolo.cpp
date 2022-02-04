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

#include "controllers/AppController.h"
#include "controllers/MappingController.h"
#include "controllers/SessionController.h"
#include "gui/views/EmptyContentView.h"
#include "gui/AudioIOPanelView.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/ConnectionGrid.h"
#include "gui/views/ControllerDevicesView.h"
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
#include "gui/LookAndFeel.h"
#include "gui/PluginManagerComponent.h"
#include "gui/views/SessionSettingsView.h"
#include "gui/views/GraphSettingsView.h"
#include "gui/views/VirtualKeyboardView.h"
#include "gui/TempoAndMeterBar.h"
#include "gui/TransportBar.h"
#include "gui/NavigationConcertinaPanel.h"

#include "session/DeviceManager.h"
#include "session/PluginManager.h"
#include "session/Node.h"

#include "Commands.h"
#include "Globals.h"
#include "Settings.h"

#include "gui/ContentComponentSolo.h"

#ifndef EL_USE_ACCESSORY_BUTTONS
 #define EL_USE_ACCESSORY_BUTTONS 0
#endif

#ifndef EL_USE_GRAPH_MIXER_VIEW
 #define EL_USE_GRAPH_MIXER_VIEW 1
#endif

#ifndef EL_USE_NODE_CHANNEL_STRIP
 #define EL_USE_NODE_CHANNEL_STRIP 1
#endif

#define EL_NAV_MIN_WIDTH 100
#define EL_NAV_MAX_WIDTH 440

namespace Element {

class SmartLayoutManager : public StretchableLayoutManager {
public:
    SmartLayoutManager() { }
    virtual ~SmartLayoutManager() { }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartLayoutManager);
};

class SmartLayoutResizeBar : public StretchableLayoutResizerBar
{
public:
    Signal<void()> mousePressed, mouseReleased;
    SmartLayoutResizeBar (StretchableLayoutManager* layoutToUse,
                          int itemIndexInLayout, bool isBarVertical)
    : StretchableLayoutResizerBar (layoutToUse, itemIndexInLayout, isBarVertical),
      layout (nullptr)
    {
        mousePressed.disconnect_all_slots();
        mouseReleased.disconnect_all_slots();
    }
    
    ~SmartLayoutResizeBar()
    {
        layout = nullptr;
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
    
private:
    SmartLayoutManager* layout;
};

// MARK: Content container

class ContentContainer : public Component
{
public:
    ContentContainer (ContentComponentSolo& cc, AppController& app)
        : owner (cc)
    {
        addAndMakeVisible (content1 = new ContentView());
        addAndMakeVisible (bar = new SmartLayoutResizeBar (&layout, 1, false));
        bar->mousePressed.connect (
            std::bind (&ContentContainer::updateLayout, this));
        bar->mouseReleased.connect (
            std::bind (&ContentContainer::lockLayout, this));

        addAndMakeVisible (content2 = new ContentView());
        updateLayout();
        resized();
    }
    
    virtual ~ContentContainer() { }
    
    void resized() override
    {
        if (content1 == nullptr || content2 == nullptr)
            return;
        Component* comps[] = { 0, 0, 0 };
        comps[0] = content1.get();
        comps[1] = bar.get();
        comps[2] = content2.get();
        if (content2->getHeight() >= accessoryHeightThreshold)
            lastAccessoryHeight = content2->getHeight();

        layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), true, true);
        if (locked && showAccessoryView && content2->getHeight() < accessoryHeightThreshold) {
            setShowAccessoryView (false);
            locked = false; 
        }       
    }
    
    void setNode (const Node& node)
    {
        if (auto* gdv = dynamic_cast<GraphDisplayView*> (content1.get()))
            gdv->setNode (node);
        else if (auto* grid = dynamic_cast<ConnectionGrid*> (content1.get()))
            grid->setNode (node);
        else if (auto* ed = dynamic_cast<GraphEditorView*> (content1.get()))
            ed->setNode (node);
        else if (nullptr != content1)
            content1->stabilizeContent();
    }
    
    void setMainView (ContentView* view)
    {
        if (view)
            view->initializeView (owner.getAppController());

        if (content1)
        {
            content1->willBeRemoved();
            removeChildComponent (content1);
        }

        content1 = view;
        
        if (content1)
        {
            content1->willBecomeActive();
            addAndMakeVisible (content1);
        }

        if (showAccessoryView)
        {
            resized();
        }
        else
        {
            updateLayout();
        }

        if (content1)
        {
            content1->didBecomeActive();
            content1->stabilizeContent();
        }
    }
    
    void setAccessoryView (ContentView* view)
    {
        if (view)
            view->initializeView (owner.getAppController());
        if (content2)
            removeChildComponent (content2);
       
        content2 = view;
        
        if (content2)
        {
            content2->willBecomeActive();
            addAndMakeVisible (content2);
        }

        resized();

        if (content2)
        {
            content2->didBecomeActive();
            content2->stabilizeContent();
        }
    }
    
    void setShowAccessoryView (const bool show)
    {
        if (showAccessoryView == show)
            return;
        showAccessoryView = show;

        if (show)
        {
            lastAccessoryHeight = jmax (accessoryHeightThreshold + 1, lastAccessoryHeight);
            layout.setItemLayout (0, 48, -1.0, content1->getHeight() - 4 - lastAccessoryHeight);
            layout.setItemLayout (1, barSize, barSize, barSize);
            layout.setItemLayout (2, 48, -1.0, lastAccessoryHeight);
        }
        else
        {
            if (capturedAccessoryHeight > 0 && capturedAccessoryHeight != lastAccessoryHeight)
            {
                lastAccessoryHeight = capturedAccessoryHeight;    
            }
            else
            {
                lastAccessoryHeight = content2->getHeight();
            }

            layout.setItemLayout (0, 48, -1.0, content1->getHeight());
            layout.setItemLayout (1, 0, 0, 0);
            layout.setItemLayout (2, 0, -1.0, 0);
            capturedAccessoryHeight = -1;
        }
        
        resized();

        if (show)
        {
        }

        locked = false;
        owner.getAppController().findChild<GuiController>()->refreshMainMenu();
    }
    
    void saveState (PropertiesFile* props)
    {
        props->setValue ("ContentContainer_width", getWidth());
        props->setValue ("ContentContainer_height", getHeight());
        props->setValue ("ContentContainer_height1", content1->getHeight());
        props->setValue ("ContentContainer_height2", content2->getHeight());
    }

    void restoreState (PropertiesFile* props)
    {
        setSize (props->getIntValue ("ContentContainer_width",  jmax (48, getWidth())),
                 props->getIntValue ("ContentContainer_height", jmax (48, getHeight())));
        content1->setSize (getWidth(), props->getIntValue ("ContentContainer_height1", 48));
        content2->setSize (getWidth(), props->getIntValue ("ContentContainer_height2", lastAccessoryHeight));
        lastAccessoryHeight = content2->getHeight();
        updateLayout();
    }

private:
    friend class ContentComponentSolo;
    ContentComponentSolo& owner;
    StretchableLayoutManager layout;
    ScopedPointer<SmartLayoutResizeBar> bar;
    ScopedPointer<ContentView> content1;
    ScopedPointer<ContentView> content2;
    
    bool showAccessoryView = false;
    int barSize = 2;
    int lastAccessoryHeight = 172;
    int capturedAccessoryHeight = -1;
    const int accessoryHeightThreshold = 50;
    bool locked = true;

    void lockLayout()
    {
        locked = true;
        
        const int content1Min = 48;
        if (showAccessoryView)
        {
            layout.setItemLayout (0, content1Min, -1.0, content1->getHeight());
            layout.setItemLayout (1, barSize, barSize, barSize);
            layout.setItemLayout (2, content2->getHeight(), content2->getHeight(), content2->getHeight());
        }
        else
        {
            layout.setItemLayout (0, content1Min, -1.0, content1->getHeight());
            layout.setItemLayout (1, 0, 0, 0);
            layout.setItemLayout (2, 0, -1.0, 0);
        }
       
        resized();

        if (showAccessoryView)
        {
            capturedAccessoryHeight = content2->getHeight();
        }
    }

    void updateLayout()
    {
        locked = false;

        if (showAccessoryView)
        {
            layout.setItemLayout (0, 48, -1.0, content1->getHeight());
            layout.setItemLayout (1, barSize, barSize, barSize);
            layout.setItemLayout (2, 48, -1.0, content2->getHeight());
        }
        else
        {
            layout.setItemLayout (0, 200, -1.0, 200);
            layout.setItemLayout (1, 0, 0, 0);
            layout.setItemLayout (2, 0, -1.0, 0);
        }

        resized();

        if (showAccessoryView)
        {
            capturedAccessoryHeight = content2->getHeight();
        }
    }
};

class ContentComponentSolo::Resizer : public StretchableLayoutResizerBar
{
public:
    Resizer (ContentComponentSolo& ContentComponentSolo, StretchableLayoutManager* layoutToUse,
             int itemIndexInLayout, bool isBarVertical)
    : StretchableLayoutResizerBar (layoutToUse, itemIndexInLayout, isBarVertical),
      owner (ContentComponentSolo)
    { }
    
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
    ContentComponentSolo& owner;
};

static ContentView* createLastContentView (Settings& settings)
{
    auto* props = settings.getUserSettings();
    const String lastContentView = props->getValue ("lastContentView");
    ScopedPointer<ContentView> view;
    typedef GraphEditorView DefaultView;

    if (lastContentView.isEmpty())
        view = new DefaultView();
    else if (lastContentView == "PatchBay")
        view = new ConnectionGrid();
    else if (lastContentView == "GraphEditor")
        view = new GraphEditorView();
    else if (lastContentView == "ControllerDevicesView")
        view = new ControllerDevicesView();
    else
        view = new DefaultView();
    
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

static void windowSizeProperty (Settings& settings, const String& property, int& w, int& h,
                                const int defaultW, const int defaultH)
{
    auto* props = settings.getUserSettings();
    StringArray tokens;
    tokens.addTokens (props->getValue(property).trim(), false);
    tokens.removeEmptyStrings();
    tokens.trim();

    w = defaultW; h = defaultH;

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

ContentComponentSolo::ContentComponentSolo (AppController& ctl_)
    : ContentComponent (ctl_)
{
    auto& settings (getGlobals().getSettings());

    setOpaque (true);
    
    addAndMakeVisible (nav = new NavigationConcertinaPanel (ctl_.getWorld()));
    nav->updateContent();
    addAndMakeVisible (bar1 = new Resizer (*this, &layout, 1, true));
    addAndMakeVisible (container = new ContentContainer (*this, getAppController()));
    
    {
        int w, h;
        windowSizeProperty (settings, "mainWindowState", w, h, 760, 480);
        setSize (w, h);
        updateLayout();
        resized();
    }

    container->setMainView (createLastContentView (settings));
    
   #if defined (EL_PRO)
    if (booleanProperty (settings, "accessoryView", false))
    {
        setAccessoryView (stringProperty (settings, "accessoryViewName", EL_VIEW_GRAPH_MIXER));
    }
    else
    {
        setShowAccessoryView (false);
    }
   #endif

    setVirtualKeyboardVisible (booleanProperty (settings, "virtualKeyboard", false));
   
   #if defined (EL_PRO) || defined (EL_SOLO) 
    setNodeChannelStripVisible (booleanProperty (settings, "channelStrip", false));
   #else
    setNodeChannelStripVisible (false);
   #endif

    const Node node (getGlobals().getSession()->getCurrentGraph());
    setCurrentNode (node);
    
    toolBarVisible = true;
    toolBarSize = 32;
    statusBarVisible = true;
    statusBarSize = 22;
    
    {
        int navSize = settings.getUserSettings()->getIntValue ("navSize", 220);
        nav->setSize (navSize, getHeight());
        resizerMouseUp();
    }
   #ifdef EL_PRO
    nav->setPanelSize (nav->getSessionPanel(), 20 * 6, false);
   #endif
    nav->setPanelSize (nav->getPluginsPanel(), 20 * 4, false);

   #if defined (EL_SOLO) || defined (EL_FREE)
    setShowAccessoryView (false);
   #endif

   #if defined (EL_FREE)
    setNodeChannelStripVisible (false);
   #endif

    resized();
}

ContentComponentSolo::~ContentComponentSolo() noexcept
{
    setContentView (nullptr, false);
    setContentView (nullptr, true);
}

String ContentComponentSolo::getMainViewName() const
{
    if (container && container->content1)
        return container->content1->getName();
    return String();
}

String ContentComponentSolo::getAccessoryViewName() const
{
    if (container && container->content2)
        return container->content2->getName();
    return String();
}

int ContentComponentSolo::getNavSize()
{
    return nav != nullptr ? nav->getWidth() : 220;
}

void ContentComponentSolo::setMainView (const String& name)
{
    if (name == "PatchBay") {
        setContentView (new ConnectionGrid());
    } else if (name == "GraphEditor" || name == "GraphEditorView") {
        setContentView (new GraphEditorView());
    } else if (name == "PluginManager") {
        setContentView (new PluginManagerContentView());
    } else if (name == "SessionSettings" || name == "SessionProperties") {
        setContentView (new SessionContentView());
    } else if (name == "GraphSettings") {
        setContentView (new GraphSettingsView());
    } else if (name == "KeymapEditorView") {
        setContentView (new KeymapEditorView());
    } else if (name == "ControllerDevicesView") {
        setContentView (new ControllerDevicesView());
    }
    else
    {
        if (auto s = getGlobals().getSession())
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

void ContentComponentSolo::backMainView()
{
    setMainView (lastMainView.isEmpty() ? "GraphEditor" : lastMainView);
}

void ContentComponentSolo::nextMainView()
{
    // only have two rotatable views as of now
    if (getMainViewName() == "EmptyView")
        return;
    const String nextName = getMainViewName() == "GraphEditor" ? "PatchBay" : "GraphEditor";
    setMainView (nextName);
}

void ContentComponentSolo::setContentView (ContentView* view, const bool accessory)
{
    jassert (container != nullptr);
    std::unique_ptr<ContentView> deleter (view);
    if (accessory)
    {
        container->setAccessoryView (deleter.release());
    }
    else
    {
        lastMainView = getMainViewName();
        container->setMainView (deleter.release());
    }
}

void ContentComponentSolo::setAccessoryView (const String& name)
{
    if (name == "PatchBay") {
        setContentView (new ConnectionGrid(), true);
    } else if (name == EL_VIEW_GRAPH_MIXER) {
        setContentView (new GraphMixerView(), true);
    } else if (name == EL_VIEW_CONSOLE) {
        setContentView (new LuaConsoleView(), true);
    }

    container->setShowAccessoryView (true);
}

void ContentComponentSolo::resizeContent (const Rectangle<int>& area)
{
    Rectangle<int> r (area);
    
    if (virtualKeyboardVisible && keyboard)
        keyboard->setBounds (r.removeFromBottom (virtualKeyboardSize));
    if (nodeStrip && nodeStrip->isVisible())
        nodeStrip->setBounds (r.removeFromRight (nodeStripSize));

    Component* comps [3] = { nav.get(), bar1.get(), container.get() };
    layout.layOutComponents (comps, 3, r.getX(), r.getY(),
                             r.getWidth(), r.getHeight(),
                             false, true);
}

bool ContentComponentSolo::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    const auto& desc (dragSourceDetails.description);
    return desc.toString() == "ccNavConcertinaPanel" || 
        (desc.isArray() && desc.size() >= 2 && desc[0] == "plugin");
}

void ContentComponentSolo::itemDropped (const SourceDetails& dragSourceDetails)
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
        auto& list (getGlobals().getPluginManager().getKnownPlugins());
        if (auto plugin = list.getTypeForIdentifierString (desc[1].toString()))
            this->post (new LoadPluginMessage (*plugin, true));
        else
            AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon, "Could not load plugin",
                                              "The plugin you dropped could not be loaded for an unknown reason.");
    }
}

bool ContentComponentSolo::isInterestedInFileDrag (const StringArray &files)
{
    for (const auto& path : files)
    {
        const File file (path);
        if (file.hasFileExtension ("elc;elg;els;dll;vst3;vst;elpreset"))
            return true;
    }
    return false;
}

void ContentComponentSolo::filesDropped (const StringArray &files, int x, int y)
{
    for (const auto& path : files)
    {
        const File file (path);
        if (file.hasFileExtension ("elc"))
        {
            #if 0
            auto& unlock (getGlobals().getUnlockStatus());
            FileInputStream src (file);
            if (unlock.applyKeyFile (src.readString()))
            {
                unlock.save();
                unlock.loadAll();
                unlock.refreshed();
                stabilizeViews();
                AlertWindow::showMessageBox (AlertWindow::InfoIcon, "Apply License File", 
                    "Your software has successfully been unlocked.");

            }
            else
            {
                AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                    "Apply License File", "Your software could not be unlocked.");
            }
            #endif
        }
        else if (file.hasFileExtension ("els"))
        {
            this->post (new OpenSessionMessage (file));
        }
        else if (file.hasFileExtension ("elg"))
        {
            if (auto* sess = getAppController().findChild<SessionController>())
                sess->importGraph (file);
        }
        else if (file.hasFileExtension ("elpreset"))
        {
            const auto data = Node::parse (file);
            if (data.hasType (Tags::node))
            {
                const Node node (data, false);
                this->post (new AddNodeMessage (node));
            }
            else
            {
                AlertWindow::showMessageBox(AlertWindow::InfoIcon, "Presets", "Error adding preset");
            }
        }
        else if ((file.hasFileExtension ("dll") || file.hasFileExtension ("vst") || file.hasFileExtension ("vst3")) &&
                 (getMainViewName() == "GraphEditor" || getMainViewName() == "PatchBay" || getMainViewName() == "PluginManager"))
        {
            auto s = getSession();
            auto graph = s->getActiveGraph();
            PluginDescription desc;
            desc.pluginFormatName = file.hasFileExtension("vst3") ? "VST3" : "VST";
            desc.fileOrIdentifier = file.getFullPathName();
#if 0
            this->post(new LoadPluginMessage(desc, false));
#endif
            auto message = std::make_unique<AddPluginMessage>(graph, desc);
            auto& builder(message->builder);

            if (ModifierKeys::getCurrentModifiersRealtime().isAltDown())
            {
                const auto audioInputNode = graph.getIONode(PortType::Audio, true);
                const auto midiInputNode = graph.getIONode(PortType::Midi, true);
                builder.addChannel(audioInputNode, PortType::Audio, 0, 0, false);
                builder.addChannel(audioInputNode, PortType::Audio, 1, 1, false);
                builder.addChannel(midiInputNode, PortType::Midi, 0, 0, false);
            }

            if (ModifierKeys::getCurrentModifiersRealtime().isCommandDown())
            {
                const auto audioOutputNode = graph.getIONode(PortType::Audio, false);
                const auto midiOutNode = graph.getIONode(PortType::Midi, false);
                builder.addChannel(audioOutputNode, PortType::Audio, 0, 0, true);
                builder.addChannel(audioOutputNode, PortType::Audio, 1, 1, true);
                builder.addChannel(midiOutNode, PortType::Midi, 0, 0, true);
            }

            this->post(message.release());
        }
    }
}

void ContentComponentSolo::stabilize (const bool refreshDataPathTrees)
{
    auto session = getGlobals().getSession();
    if (session->getNumGraphs() > 0)
    {
        const Node graph = session->getCurrentGraph();
        setCurrentNode (graph);
    }
    else
    {
        setContentView (new EmptyContentView());
    }
    
    if (auto* window = findParentComponentOfClass<DocumentWindow>())
        window->setName ("Element - " + session->getName());
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

void ContentComponentSolo::stabilizeViews()
{
    if (container->content1)
        container->content1->stabilizeContent();
    if (container->content2)
        container->content2->stabilizeContent();
    if (nodeStrip)
        nodeStrip->stabilizeContent();
}

void ContentComponentSolo::saveState (PropertiesFile* props)
{
    jassert (props);
    if (nav)
        nav->saveState (props);
    if (container)
        container->saveState (props);
    if (auto* const vk = getVirtualKeyboardView())
        vk->saveState (props);
}

void ContentComponentSolo::restoreState (PropertiesFile* props)
{
    jassert (props);
    if (nav)
        nav->restoreState (props);
    if (container)
        container->restoreState (props);
    if (auto* const vk = getVirtualKeyboardView())
        vk->restoreState (props);
    resized();
}

void ContentComponentSolo::setCurrentNode (const Node& node)
{
    if ((nullptr != dynamic_cast<EmptyContentView*> (container->content1.get()) ||
        getMainViewName() == "SessionSettings" ||
        getMainViewName() == "PluginManager" ||
        getMainViewName() == "ControllerDevicesView") &&
        getSession()->getNumGraphs() > 0)
    {
        setMainView ("GraphEditor");
    }

    container->setNode (node);
}

void ContentComponentSolo::updateLayout()
{
    layout.setItemLayout (0, EL_NAV_MIN_WIDTH, EL_NAV_MAX_WIDTH, nav->getWidth());
    layout.setItemLayout (1, 2, 2, 2);
    layout.setItemLayout (2, 100, -1, 400);
}

void ContentComponentSolo::resizerMouseDown()
{
    updateLayout();
}

void ContentComponentSolo::resizerMouseUp()
{
    layout.setItemLayout (0, nav->getWidth(), nav->getWidth(), nav->getWidth());
    layout.setItemLayout (1, 2, 2, 2);
    layout.setItemLayout (2, 100, -1, 400);
    resized();
}

void ContentComponentSolo::setVirtualKeyboardVisible (const bool isVisible)
{
    if (isVisible == virtualKeyboardVisible)
        return;
    
    if (isVisible)
    {
        if (! keyboard) keyboard = new VirtualKeyboardView();
        keyboard->willBecomeActive();
        addAndMakeVisible (keyboard);
        keyboard->didBecomeActive();
        if (keyboard->isShowing() || keyboard->isOnDesktop())
            keyboard->grabKeyboardFocus();
    }
    else
    {
        keyboard = nullptr;
    }
    
    virtualKeyboardVisible = isVisible;
    resized();
}

void ContentComponentSolo::setNodeChannelStripVisible (const bool isVisible)
{
    if (! nodeStrip)
    {
        nodeStrip = new NodeChannelStripView();
        nodeStrip->initializeView (getAppController());
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

bool ContentComponentSolo::isNodeChannelStripVisible() const { return nodeStrip && nodeStrip->isVisible(); }

void ContentComponentSolo::toggleVirtualKeyboard()
{
    setVirtualKeyboardVisible (! virtualKeyboardVisible);
}

ApplicationCommandTarget* ContentComponentSolo::getNextCommandTarget()
{
    return (container) ? container->content1.get() : nullptr;
}

void ContentComponentSolo::setShowAccessoryView (const bool show)
{
    if (container) container->setShowAccessoryView (show);
}

bool ContentComponentSolo::showAccessoryView() const
{
    return (container) ? container->showAccessoryView : false;
}

void ContentComponentSolo::getSessionState (String& state)
{
    ValueTree data ("state");
    
    if (auto* const ned = nav->findPanel<NodeEditorContentView>())
    {
        String nedState; ned->getState (nedState);
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

void ContentComponentSolo::applySessionState (const String& state)
{
    MemoryBlock mb; mb.fromBase64Encoding (state);
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

void ContentComponentSolo::setMainView (ContentView* view)
{
    jassert (view != nullptr);
    setContentView (view, false);
}

}
