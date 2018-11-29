/*
    ContentComponent.cpp - This file is part of Element
    Copyright (C) 2015-2017  Kushview, LLC.  All rights reserved.
*/

#include "controllers/AppController.h"
#include "controllers/SessionController.h"

#include "engine/GraphProcessor.h"

#include "gui/AudioIOPanelView.h"
#include "gui/PluginsPanelView.h"
#include "gui/ConnectionGrid.h"
#include "gui/ControllerDevicesView.h"
#include "gui/GraphEditorView.h"
#include "gui/GraphMixerView.h"
#include "gui/KeymapEditorView.h"
#include "gui/NodeChannelStripView.h"
#include "gui/MainWindow.h"
#include "gui/MainMenu.h"
#include "gui/NavigationView.h"
#include "gui/SessionTreePanel.h"
#include "gui/ViewHelpers.h"
#include "gui/LookAndFeel.h"
#include "gui/PluginManagerComponent.h"
#include "gui/SessionSettingsView.h"
#include "gui/GraphSettingsView.h"
#include "gui/VirtualKeyboardView.h"
#include "gui/TempoAndMeterBar.h"
#include "gui/TransportBar.h"
#include "gui/NavigationConcertinaPanel.h"

#include "session/DeviceManager.h"
#include "session/PluginManager.h"
#include "session/Node.h"
#include "session/UnlockStatus.h"

#include "Commands.h"
#include "Globals.h"
#include "Settings.h"

#include "gui/ContentComponent.h"

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
#define EL_NAV_MAX_WIDTH 360

namespace Element {

static void showProductLockedAlert (const String& msg = String(), const String& title = "Feature not Available")
{
    String message = (msg.isEmpty()) ? "Unlock the full version of Element to use this feature.\nGet a copy @ https://kushview.net"
        : msg;
    if (AlertWindow::showOkCancelBox (AlertWindow::InfoIcon, title, message, "Upgrade", "Cancel"))
        URL("https://kushview.net/products/element/").launchInDefaultBrowser();
}

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

// MARK: Content View

ContentView::ContentView()
{
    addKeyListener (this);
}
    
ContentView::~ContentView()
{
    removeKeyListener (this);
}

void ContentView::paint (Graphics& g) { g.fillAll (LookAndFeel::contentBackgroundColor); }
    
bool ContentView::keyPressed (const KeyPress& k, Component*)
{
    if (escapeTriggersClose && k == KeyPress::escapeKey)
    {
        ViewHelpers::invokeDirectly (this, Commands::showLastContentView, true);
        return true;
    }
    
    return false;
}

void ContentView::disableIfNotUnlocked()
{
    if (auto* w = ViewHelpers::getGlobals (this))
    {
        setEnabled (w->getUnlockStatus().isFullVersion());
    }
    else
    {
        jassertfalse;
        setEnabled (false);
    }

    setInterceptsMouseClicks (isEnabled(), isEnabled());
}

// MARK: Toolbar
    
class ContentComponent::Toolbar : public Component,
                                  public Button::Listener,
                                  public Timer
{
public:
    Toolbar (ContentComponent& o)
        : owner(o), viewBtn ("e")
    {
        addAndMakeVisible (viewBtn);
        viewBtn.setButtonText ("view");
       #if EL_USE_ACCESSORY_BUTTONS
        
        addAndMakeVisible (panicBtn);
       #endif
       #if EL_RUNNING_AS_PLUGIN
        addAndMakeVisible (menuBtn);
       #endif
        for (auto* b : { (Button*)&viewBtn, (Button*)&panicBtn, (Button*)&menuBtn })
            b->addListener (this);
        addAndMakeVisible (tempoBar);
        addAndMakeVisible (transport);

        mapButton.setButtonText ("map");
        mapButton.setColour (SettingButton::backgroundOnColourId, Colors::toggleBlue);
        mapButton.addListener (this);
        addAndMakeVisible (mapButton);
    }
    
    ~Toolbar() { }
    
    void setSession (SessionPtr s)
    {
        session = s;
        auto& status (ViewHelpers::getGlobals(this)->getUnlockStatus());
        auto& settings (ViewHelpers::getGlobals(this)->getSettings());
        auto* props = settings.getUserSettings();
        
       #if ! EL_RUNNING_AS_PLUGIN
        const bool showExt = props->getValue ("clockSource") == "midiClock" && status.isFullVersion();
       #else
        // Plugin always has host sync option
        const bool showExt = true;
        ignoreUnused (props, status);
       #endif
       
        if (session)
        {
            tempoBar.setUseExtButton (showExt);
            tempoBar.getTempoValue().referTo (session->getPropertyAsValue (Tags::tempo));
            tempoBar.getExternalSyncValue().referTo (session->getPropertyAsValue ("externalSync"));
            tempoBar.stabilizeWithSession (false);
        }
        
        mapButton.setEnabled ((bool) status.isFullVersion());

        resized();
    }
    
    void resized() override
    {
        Rectangle<int> r (getLocalBounds());
        
        const int tempoBarWidth = jmax (120, tempoBar.getWidth());
        const int tempoBarHeight = getHeight() - 16;
        
        tempoBar.setBounds (10, 8, tempoBarWidth, tempoBarHeight);
        
        r.removeFromRight (10);
        
        if (menuBtn.isVisible())
        {
            menuBtn.setBounds (r.removeFromRight(tempoBarHeight)
                   .withSizeKeepingCentre(tempoBarHeight, tempoBarHeight));
            r.removeFromRight (4);
        }
        
        if (panicBtn.isVisible())
        {
            panicBtn.setBounds (r.removeFromRight(tempoBarHeight)
                    .withSizeKeepingCentre(tempoBarHeight, tempoBarHeight));
            r.removeFromRight (4);
        }
        
        if (viewBtn.isVisible())
        {
            viewBtn.setBounds (r.removeFromRight(tempoBarHeight * 2)
                                .withSizeKeepingCentre(tempoBarHeight * 2, tempoBarHeight));
        }
        
        if (mapButton.isVisible())
        {
            r.removeFromRight (4);
            mapButton.setBounds (r.removeFromRight (tempoBarHeight * 2)
                                  .withSizeKeepingCentre (tempoBarHeight * 2, tempoBarHeight));
        }

        if (transport.isVisible())
        {
            r = getLocalBounds().withX ((getWidth() / 2) - (transport.getWidth() / 2));
            r.setWidth (transport.getWidth());
            transport.setBounds (r.withSizeKeepingCentre (r.getWidth(), tempoBarHeight));
        }
    }
    
    void paint (Graphics& g) override
    {
        g.setColour (LookAndFeel_KV1::contentBackgroundColor.brighter (0.1));
        g.fillRect (getLocalBounds());
    }
    
    void buttonClicked (Button* btn) override
    {
        auto& status (ViewHelpers::getGlobals(this)->getUnlockStatus());
        if (btn == &viewBtn)
        {
            const int command = owner.getMainViewName() == "PatchBay" || owner.getMainViewName() == "GraphEditor"
                              ? Commands::rotateContentView : Commands::showLastContentView;
            ViewHelpers::invokeDirectly (this, command, true);
        }
        else  if (btn == &panicBtn)
        {
            ViewHelpers::invokeDirectly (this, Commands::panic, true);
        }
        else if (btn == &menuBtn)
        {
            PopupMenu menu;
            if (auto* cc = ViewHelpers::findContentComponent (this))
                MainMenu::buildPluginMainMenu (cc->getGlobals().getCommandManager(), menu);
            if (99999 == menu.show())
                ViewHelpers::closePluginWindows (this, false);
        }
        else if (btn == &mapButton)
        {
            if ((bool) status.isFullVersion())
            {
                if (auto* mapping = owner.getAppController().findChild<MappingController>())
                {
                    mapping->learn (! mapButton.getToggleState());
                    mapButton.setToggleState (mapping->isLearning(), dontSendNotification);
                    if (mapping->isLearning()) {
                        startTimer (600);
                    }
                }
            }
        }
    }

    void timerCallback() override
    {
        if (auto* mapping = owner.getAppController().findChild<MappingController>())
        {
            if (! mapping->isLearning())
            {
                mapButton.setToggleState (false, dontSendNotification);
                stopTimer();
            }
        }
    }

private:
    ContentComponent& owner;
    SessionPtr session;
    SettingButton menuBtn;
    SettingButton viewBtn;
    SettingButton mapButton;
    PanicButton panicBtn;
    TempoAndMeterBar tempoBar;
    TransportBar     transport;
};

class ContentComponent::StatusBar : public Component,
                                    public Value::Listener,
                                    private Timer
{
public:
    StatusBar (Globals& g)
        : world (g),
          devices (world.getDeviceManager()),
          plugins (world.getPluginManager())
    {
        sampleRate.addListener (this);
        streamingStatus.addListener (this);
        status.addListener (this);
        
        addAndMakeVisible (sampleRateLabel);
        addAndMakeVisible (streamingStatusLabel);
        addAndMakeVisible (statusLabel);
        
        const Colour labelColor (0xffaaaaaa);
        const Font font (12.0f);
        
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (Label* label = dynamic_cast<Label*> (getChildComponent (i)))
            {
                label->setFont (font);
                label->setColour (Label::textColourId, labelColor);
                label->setJustificationType (Justification::centredLeft);
            }
        }
        
        startTimer (2000);
        updateLabels();
    }
    
    ~StatusBar()
    {
        sampleRate.removeListener (this);
        streamingStatus.removeListener (this);
        status.removeListener (this);
    }
    
    void paint (Graphics& g) override
    {
        g.setColour (LookAndFeel_KV1::contentBackgroundColor.brighter(0.1));
        g.fillRect (getLocalBounds());
        
        const Colour lineColor (0xff545454);
        g.setColour (lineColor);
        
        g.drawLine(streamingStatusLabel.getX(), 0, streamingStatusLabel.getX(), getHeight());
        g.drawLine(sampleRateLabel.getX(), 0, sampleRateLabel.getX(), getHeight());
        g.setColour (lineColor.darker());
        g.drawLine (0, 0, getWidth(), 0);
        g.setColour (lineColor);
        g.drawLine (0, 1, getWidth(), 1);
    }
    
    void resized() override
    {
        Rectangle<int> r (getLocalBounds());
        statusLabel.setBounds (r.removeFromLeft (getWidth() / 5));
        streamingStatusLabel.setBounds (r.removeFromLeft (r.getWidth() / 2));
        sampleRateLabel.setBounds(r);
    }
    
    void valueChanged (Value&) override
    {
        updateLabels();
    }
    
    void updateLabels()
    {
        auto engine = world.getAudioEngine();
       #if EL_RUNNING_AS_PLUGIN
        String text = "Latency: ";
        const int latencySamples = (engine != nullptr) ? engine->getExternalLatencySamples() : 0;
        text << latencySamples << " samples";
        sampleRateLabel.setText (text, dontSendNotification);
        streamingStatusLabel.setText ("", dontSendNotification);
        statusLabel.setText ("Plugin", dontSendNotification);
        
       #else
        if (auto* dev = devices.getCurrentAudioDevice())
        {
            String text = "Sample Rate: ";
            text << String (dev->getCurrentSampleRate() * 0.001, 1) << " KHz";
            text << ":  Buffer: " << dev->getCurrentBufferSizeSamples();
            sampleRateLabel.setText (text, dontSendNotification);
            
            text.clear();
            String strText = streamingStatus.getValue().toString();
            if (strText.isEmpty())
                strText = "Running";
            text << "Engine: " << strText << ":  CPU: " << String(devices.getCpuUsage() * 100.f, 1) << "%";
            streamingStatusLabel.setText (text, dontSendNotification);
            
            statusLabel.setText (String("Device: ") + dev->getName(), dontSendNotification);
        }
        else
        {
            sampleRateLabel.setText ("", dontSendNotification);
            streamingStatusLabel.setText ("", dontSendNotification);
            statusLabel.setText ("No Device", dontSendNotification);
        }

		if (plugins.isScanningAudioPlugins())
		{
			auto text = streamingStatusLabel.getText();
			auto name = plugins.getCurrentlyScannedPluginName();
			name = File::createFileWithoutCheckingPath(name).getFileName();

			text << " - Scanning: " << name;
			if (name.isNotEmpty())
				streamingStatusLabel.setText(text, dontSendNotification);
		}
       #endif
    }
    
private:
    Globals& world;
    DeviceManager& devices;
    PluginManager& plugins;
    
    Label sampleRateLabel, streamingStatusLabel, statusLabel;
    ValueTree node;
    Value sampleRate, streamingStatus, status;
    
    friend class Timer;
    void timerCallback() override {
        updateLabels();
    }
};

class EmptyContentView : public ContentView
{
public:
    void paint (Graphics& g) override
    {
        setName ("EmptyView");
        g.fillAll (LookAndFeel::contentBackgroundColor);
        g.setColour (LookAndFeel::textColor);
        g.setFont (16.f);
        
       #if JUCE_MAC
        const String msg ("Session is empty.\nPress Shift+Cmd+N to add a graph.");
       #else
        const String msg ("Session is empty.\nPress Shift+Ctl+N to add a graph.");
       #endif
        g.drawFittedText (msg, 0, 0, getWidth(), getHeight(), Justification::centred, 2);
    }
};

// MARK: Content container

class ContentContainer : public Component
{
public:
    ContentContainer (ContentComponent& cc, AppController& app)
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

        content1->didBecomeActive();
        content1->stabilizeContent();
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
        content2->didBecomeActive();
        content2->stabilizeContent();
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
    friend class ContentComponent;
    ContentComponent& owner;
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

class ContentComponent::Resizer : public StretchableLayoutResizerBar
{
public:
    Resizer (ContentComponent& contentComponent, StretchableLayoutManager* layoutToUse,
             int itemIndexInLayout, bool isBarVertical)
    : StretchableLayoutResizerBar (layoutToUse, itemIndexInLayout, isBarVertical),
      owner (contentComponent)
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
    ContentComponent& owner;
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

struct ContentComponent::Tooltips
{
    Tooltips() { tooltipWindow = new TooltipWindow(); }
    ScopedPointer<TooltipWindow> tooltipWindow;
};

ContentComponent::ContentComponent (AppController& ctl_)
    : controller (ctl_)
{
    auto& settings (controller.getGlobals().getSettings());

    setOpaque (true);
    
    addAndMakeVisible (nav = new NavigationConcertinaPanel (ctl_.getWorld()));
    nav->updateContent();
    addAndMakeVisible (bar1 = new Resizer (*this, &layout, 1, true));
    addAndMakeVisible (container = new ContentContainer (*this, controller));
    addAndMakeVisible (statusBar = new StatusBar (getGlobals()));
    addAndMakeVisible (toolBar = new Toolbar (*this));
    
    {
        int w, h;
        windowSizeProperty (settings, "mainWindowState", w, h, 760, 480);
        setSize (w, h);
        updateLayout();
        resized();
    }

    container->setMainView (createLastContentView (settings));
    
    if (booleanProperty (settings, "accessoryView", false))
    {
        setAccessoryView (stringProperty (settings, "accessoryViewName", EL_VIEW_GRAPH_MIXER));
    }
    else
    {
        setShowAccessoryView (false);
    }
    
    setVirtualKeyboardVisible (booleanProperty (settings, "virtualKeyboard", false));    
    setNodeChannelStripVisible (booleanProperty (settings, "channelStrip", false));

    const Node node (getGlobals().getSession()->getCurrentGraph());
    setCurrentNode (node);
    
    toolBarVisible = true;
    toolBarSize = 32;
    statusBarVisible = true;
    statusBarSize = 22;
    
    {
        int navSize = controller.getGlobals().getSettings().getUserSettings()->getIntValue ("navSize", 220);
        nav->setSize (navSize, getHeight());
        resizerMouseUp();
    }
    
    toolBar->setSession (getGlobals().getSession());
    nav->setPanelSize (nav->getSessionPanel(), 20 * 6, false);
    nav->setPanelSize (nav->getPluginsPanel(), 20 * 4, false);
//    nav->setPanelSize (nav->getUserDataPathPanel(), 60, false);


   #ifdef EL_FREE
    setNodeChannelStripVisible (false);
    setShowAccessoryView (false);
   #endif
    resized();
}

ContentComponent::~ContentComponent() noexcept
{
}

Globals& ContentComponent::getGlobals() { return controller.getGlobals(); }
SessionPtr ContentComponent::getSession() { return getGlobals().getSession(); }

String ContentComponent::getMainViewName() const
{
    if (container && container->content1)
        return container->content1->getName();
    return String();
}

String ContentComponent::getAccessoryViewName() const
{
    if (container && container->content2)
        return container->content2->getName();
    return String();
}

int ContentComponent::getNavSize()
{
    return nav != nullptr ? nav->getWidth() : 220;
}

void ContentComponent::childBoundsChanged (Component* child) { }
void ContentComponent::mouseDown (const MouseEvent& ev) { Component::mouseDown (ev); }

void ContentComponent::setMainView (const String& name)
{
    if (name == "PatchBay") {
        setContentView (new ConnectionGrid());
    } else if (name == "GraphEditor") {
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
        if (auto s = controller.getWorld().getSession())
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

void ContentComponent::backMainView()
{
    setMainView (lastMainView.isEmpty() ? "GraphEditor" : lastMainView);
}

void ContentComponent::nextMainView()
{
    // only have two rotatable views as of now
    if (getMainViewName() == "EmptyView")
        return;
    const String nextName = getMainViewName() == "GraphEditor" ? "PatchBay" : "GraphEditor";
    setMainView (nextName);
}

void ContentComponent::setContentView (ContentView* view, const bool accessory)
{
    jassert (view && container);
    ScopedPointer<ContentView> deleter (view);
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

void ContentComponent::setAccessoryView (const String& name)
{
    if (name == "PatchBay") {
        setContentView (new ConnectionGrid(), true);
    } else if (name == EL_VIEW_GRAPH_MIXER) {
        setContentView (new GraphMixerView(), true);
    }

    container->setShowAccessoryView (true);
}

void ContentComponent::paint (Graphics &g)
{
    g.fillAll (LookAndFeel::backgroundColor);
}

void ContentComponent::resized()
{
    Rectangle<int> r (getLocalBounds());
    
    if (toolBarVisible && toolBar)
        toolBar->setBounds (r.removeFromTop (toolBarSize));
    if (statusBarVisible && statusBar)
        statusBar->setBounds (r.removeFromBottom (statusBarSize));
    if (virtualKeyboardVisible && keyboard)
        keyboard->setBounds (r.removeFromBottom (virtualKeyboardSize));
    if (nodeStrip && nodeStrip->isVisible())
        nodeStrip->setBounds (r.removeFromRight (nodeStripSize));

    Component* comps [3] = { nav.get(), bar1.get(), container.get() };
    layout.layOutComponents (comps, 3, r.getX(), r.getY(),
                             r.getWidth(), r.getHeight(),
                             false, true);
}

bool ContentComponent::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    const auto& desc (dragSourceDetails.description);
    return desc.toString() == "ccNavConcertinaPanel" || 
        (desc.isArray() && desc.size() >= 2 && desc[0] == "plugin");
}

void ContentComponent::itemDropped (const SourceDetails& dragSourceDetails)
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
        if (auto* plugin = list.getTypeForIdentifierString (desc[1].toString()))
            this->post (new LoadPluginMessage (*plugin, true));
        else
            AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon, "Could not load plugin",
                                              "The plugin you dropped could not be loaded for an unknown reason.");
    }
}
    
bool ContentComponent::isInterestedInFileDrag (const StringArray &files)
{
    for (const auto& path : files)
    {
        const File file (path);
        if (file.hasFileExtension ("elc;elg;els;dll;vst3;vst;elpreset"))
            return true;
    }
    return false;
}

void ContentComponent::filesDropped (const StringArray &files, int x, int y)
{
    for (const auto& path : files)
    {
        const File file (path);
        if (file.hasFileExtension ("elc"))
        {
            auto& unlock (controller.getGlobals().getUnlockStatus());
            FileInputStream src (file);
            if (unlock.applyKeyFile (src.readString()))
            {
                unlock.save();
                unlock.loadAll();
                stabilizeViews();
                AlertWindow::showMessageBox (AlertWindow::InfoIcon, "Apply License File", 
                    "Your software has successfully been unlocked.");
            }
            else
            {
                AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                    "Apply License File", "Your software could not be unlocked.");
            }
        }
        else if (file.hasFileExtension ("els"))
        {
            this->post (new OpenSessionMessage (file));
        }
        else if (file.hasFileExtension ("elg"))
        {
            if (getGlobals().getUnlockStatus().isFullVersion())
            {
                if (auto* sess = controller.findChild<SessionController>())
                    sess->importGraph (file);
            }
            else
            {
                showProductLockedAlert();
            }
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
            PluginDescription desc;
            desc.pluginFormatName = file.hasFileExtension ("vst3") ? "VST3" : "VST";
            desc.fileOrIdentifier = file.getFullPathName();
            this->post (new LoadPluginMessage (desc, false));
        }
    }
}

void ContentComponent::post (Message* message)
{
    controller.postMessage (message);
}

void ContentComponent::stabilize (const bool refreshDataPathTrees)
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
    if (auto* sp = nav->getGraphsPanel())
        sp->setSession (session);
    if (auto* ss = nav->findPanel<SessionTreePanel>())
        ss->setSession (session);
    if (auto* ncv = nav->findPanel<NodeContentView>())
        ncv->stabilizeContent();
        
    toolBar->setSession (session);
    
    stabilizeViews();
    
    if (auto* main = findParentComponentOfClass<MainWindow>())
        main->refreshMenu();
    
    if (refreshDataPathTrees)
        if (auto* data = nav->findPanel<DataPathTreeComponent>())
            data->refresh();
}

void ContentComponent::stabilizeViews()
{
    if (container->content1)
        container->content1->stabilizeContent();
    if (container->content2)
        container->content2->stabilizeContent();
    if (nodeStrip)
        nodeStrip->stabilizeContent();
}

void ContentComponent::saveState (PropertiesFile* props)
{
    jassert (props);
    if (nav)
        nav->saveState (props);
    if (container)
        container->saveState (props);
}

void ContentComponent::restoreState (PropertiesFile* props)
{
    jassert (props);
    if (nav)
        nav->restoreState (props);
    if (container)
        container->restoreState (props);
    resized();
}

void ContentComponent::setCurrentNode (const Node& node)
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

void ContentComponent::updateLayout()
{
    layout.setItemLayout (0, EL_NAV_MIN_WIDTH, EL_NAV_MAX_WIDTH, nav->getWidth());
    layout.setItemLayout (1, 2, 2, 2);
    layout.setItemLayout (2, 300, -1, 400);
}

void ContentComponent::resizerMouseDown()
{
    updateLayout();
}

void ContentComponent::resizerMouseUp()
{
    layout.setItemLayout (0, nav->getWidth(), nav->getWidth(), nav->getWidth());
    layout.setItemLayout (1, 2, 2, 2);
    layout.setItemLayout (2, 300, -1, 400);
    resized();
}

void ContentComponent::setVirtualKeyboardVisible (const bool isVisible)
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

void ContentComponent::setNodeChannelStripVisible (const bool isVisible)
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

bool ContentComponent::isNodeChannelStripVisible() const { return nodeStrip && nodeStrip->isVisible(); }

void ContentComponent::toggleVirtualKeyboard()
{
    setVirtualKeyboardVisible (! virtualKeyboardVisible);
}

ApplicationCommandTarget* ContentComponent::getNextCommandTarget()
{
    return (container) ? container->content1.get() : nullptr;
}

void ContentComponent::setShowAccessoryView (const bool show)
{
    if (container) container->setShowAccessoryView (show);
}

bool ContentComponent::showAccessoryView() const
{
    return (container) ? container->showAccessoryView : false;
}

}
