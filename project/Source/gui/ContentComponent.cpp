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
#include "gui/GraphEditorView.h"
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

#ifndef EL_USE_AUDIO_PANEL
 #define EL_USE_AUDIO_PANEL 1
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
    
// MARK: Content View

ContentView::ContentView()
{
    addKeyListener(this);
    
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

// MARK: Toolbar
    
class ContentComponent::Toolbar : public Component,
                                  public ButtonListener
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
    }

private:
    ContentComponent& owner;
    SessionPtr session;
    SettingButton menuBtn;
    SettingButton viewBtn;
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
        : devices (g.getDeviceManager()),
          plugins (g.getPluginManager())
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
        
        startTimer (5000);
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
       #if EL_RUNNING_AS_PLUGIN
        sampleRateLabel.setText ("", dontSendNotification);
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
       #endif
    }
    
private:
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
        addAndMakeVisible (bar = new StretchableLayoutResizerBar (&layout, 1, false));
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
        layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), true, true);
    }
    
    void setNode (const Node& node)
    {
        if (auto* grid = dynamic_cast<ConnectionGrid*> (content1.get()))
            grid->setNode (node);
    }
    
    void setMainView (ContentView* view)
    {
        if (content1)
            removeChildComponent (content1);
        content1 = view;
        if (content1)
        {
            content1->willBecomeActive();
            addAndMakeVisible (content1);
            content1->didBecomeActive();
        }
        updateLayout();
        resized();
    }
    
    void setAccessoryView (ContentView* view)
    {
        if (content2)
            removeChildComponent (content2);
        content2 = view;
        if (content2)
        {
            content2->willBecomeActive();
            addAndMakeVisible (content2);
            content2->didBecomeActive();
        }
        updateLayout();
        resized();
    }
    
    void setShowAccessoryView (const bool show)
    {
        if (showAccessoryView == show)
            return;
        showAccessoryView = show;
        updateLayout();
    }
    
private:
    friend class ContentComponent;
    ContentComponent& owner;
    StretchableLayoutManager layout;
    ScopedPointer<StretchableLayoutResizerBar> bar;
    ScopedPointer<ContentView> content1;
    ScopedPointer<ContentView> content2;
    
    bool showAccessoryView = false;
    int barSize = 4;
    
    void updateLayout()
    {
        if (showAccessoryView)
        {
            layout.setItemLayout (0, 200, -1.0, 200);
            layout.setItemLayout (1, barSize, barSize, barSize);
            layout.setItemLayout (2, 1, -1.0, 0);
        }
        else
        {
            layout.setItemLayout (0, 200, -1.0, 200);
            layout.setItemLayout (1, 0, 0, 0);
            layout.setItemLayout (2, 0, -1.0, 0);
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
    
    if (lastContentView.isEmpty())
        view = new ConnectionGrid();
    else if (lastContentView == "PatchBay")
        view = new ConnectionGrid();
    else if (lastContentView == "GraphEditor")
        view = new GraphEditorView();
    else
        view = new ConnectionGrid();
    
    return view ? view.release() : nullptr;
}

static bool virtualKeyboardSetting (Settings& settings)
{
    auto* props = settings.getUserSettings();
    return props == nullptr ? false : props->getBoolValue ("virtualKeyboard");
}
    
ContentComponent::ContentComponent (AppController& ctl_)
    : controller (ctl_)
{
    setOpaque (true);
    
    addAndMakeVisible (nav = new NavigationConcertinaPanel (ctl_.getWorld()));
    addAndMakeVisible (bar1 = new Resizer (*this, &layout, 1, true));
    addAndMakeVisible (container = new ContentContainer (*this, controller));
    addAndMakeVisible (statusBar = new StatusBar (getGlobals()));
    addAndMakeVisible (toolBar = new Toolbar (*this));
    
    container->setMainView (createLastContentView (controller.getGlobals().getSettings()));
    setVirtualKeyboardVisible (virtualKeyboardSetting (controller.getGlobals().getSettings()));
    
    const Node node (getGlobals().getSession()->getCurrentGraph());
    setCurrentNode (node);
    
    toolBarVisible = true;
    toolBarSize = 32;
    statusBarVisible = true;
    statusBarSize = 22;
    
    setSize (760, 480);
    
    {
        int navSize = controller.getGlobals().getSettings().getUserSettings()->getIntValue ("navSize", 220);
        nav->setSize (navSize, getHeight());
        resizerMouseUp();
    }
    
    toolBar->setSession (getGlobals().getSession());
    nav->setPanelSize (nav->findPanel<ElementsNavigationPanel>(), 160, false);
}

ContentComponent::~ContentComponent()
{
    toolTips = nullptr;
}

Globals& ContentComponent::getGlobals() { return controller.getGlobals(); }
SessionPtr ContentComponent::getSession() { return getGlobals().getSession(); }

String ContentComponent::getMainViewName() const
{
    if (container && container->content1)
        return container->content1->getName();
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
        setContentView (new ConnectionGrid());
    }
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
    
    Component* comps [3] = { nav.get(), bar1.get(), container.get() };
    layout.layOutComponents (comps, 3, r.getX(), r.getY(),
                             r.getWidth(), r.getHeight(),
                             false, true);
}

bool ContentComponent::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    return true;
}
    
void ContentComponent::itemDropped (const SourceDetails& dragSourceDetails)
{
    
    if (dragSourceDetails.description.toString() == "ccNavConcertinaPanel")
        if (auto* panel = nav->findPanel<DataPathTreeComponent>())
            filesDropped (StringArray ({ panel->getSelectedFile().getFullPathName() }),
                          dragSourceDetails.localPosition.getX(),
                          dragSourceDetails.localPosition.getY());
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
                AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                    "Apply License File", "Your software has successfully been unlocked.");
            }
            else
            {
                AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                    "Apply License File", "Your software could not be unlocked.");
            }
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
    if (auto* sp = nav->getSessionPanel())
        sp->setSession (session);
    toolBar->setSession (session);
    
    if (container->content1)
        container->content1->stabilizeContent();
    if (container->content2)
        container->content2->stabilizeContent();
    
    if (auto* main = findParentComponentOfClass<MainWindow>())
        main->refreshMenu();
    
    if (refreshDataPathTrees)
        if (auto* data = nav->findPanel<DataPathTreeComponent>())
            data->refresh();
}

void ContentComponent::setCurrentNode (const Node& node)
{
    if (nullptr != dynamic_cast<EmptyContentView*> (container->content1.get()))
        if (getSession()->getNumGraphs() > 0)
            setContentView (new ConnectionGrid());
    
    if (auto* audioPanel = nav->getAudioIOPanel())
        audioPanel->setNode (node);
    
    if (node.hasNodeType (Tags::graph))
        container->setNode (node);
}

void ContentComponent::updateLayout()
{
    layout.setItemLayout (0, EL_NAV_MIN_WIDTH, EL_NAV_MAX_WIDTH, nav->getWidth());
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 300, -1, 400);
}

void ContentComponent::resizerMouseDown()
{
    updateLayout();
    //resized();
}

void ContentComponent::resizerMouseUp()
{
    layout.setItemLayout (0, nav->getWidth(), nav->getWidth(), nav->getWidth());
    layout.setItemLayout (1, 4, 4, 4);
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
    }
    else
    {
        keyboard = nullptr;
    }
    
    virtualKeyboardVisible = isVisible;
    resized();
}

void ContentComponent::toggleVirtualKeyboard()
{
    setVirtualKeyboardVisible (! virtualKeyboardVisible);
}

}
