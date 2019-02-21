/*
    ContentComponent.cpp - This file is part of Element
    Copyright (C) 2015-2019  Kushview, LLC.  All rights reserved.
*/

#include "controllers/AppController.h"
#include "controllers/MappingController.h"
#include "controllers/SessionController.h"

#include "gui/widgets/MidiBlinker.h"
#include "gui/ContentComponentPro.h"
#include "gui/ContentComponentSolo.h"
#include "gui/LookAndFeel.h"
#include "gui/MainWindow.h"
#include "gui/MainMenu.h"
#include "gui/TempoAndMeterBar.h"
#include "gui/TransportBar.h"
#include "gui/ViewHelpers.h"

#include "session/DeviceManager.h"
#include "session/Node.h"
#include "session/PluginManager.h"
#include "session/UnlockStatus.h"

#include "Commands.h"
#include "Globals.h"
#include "Settings.h"

#include "gui/ContentComponent.h"

#ifndef EL_USE_ACCESSORY_BUTTONS
 #define EL_USE_ACCESSORY_BUTTONS 0
#endif

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
    addKeyListener (this);
}
    
ContentView::~ContentView()
{
    removeKeyListener (this);
}

ContentComponent* ContentComponent::create (AppController& controller)
{
   #if defined (EL_PRO)
    #if defined (EL_DOCKING)
     return new ContentComponentPro (controller);
    #else
     return new ContentComponentSolo (controller);
    #endif
   #else
    return new ContentComponentSolo (controller);
   #endif
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
        addAndMakeVisible (midiBlinker);
    }
    
    ~Toolbar()
    { 
        for (const auto& conn : connections)
            conn.disconnect();
        connections.clear();
    }

    void setSession (SessionPtr s)
    {
        session = s;
        auto& status (ViewHelpers::getGlobals(this)->getUnlockStatus());
        auto& settings (ViewHelpers::getGlobals(this)->getSettings());
        auto engine (ViewHelpers::getGlobals(this)->getAudioEngine());

        if (midiIOMonitor == nullptr)
        {
            midiIOMonitor = engine->getMidiIOMonitor();
            connections.add (midiIOMonitor->midiSent.connect (
                std::bind (&MidiBlinker::triggerSent, &midiBlinker)));
            connections.add (midiIOMonitor->midiReceived.connect (
                std::bind (&MidiBlinker::triggerReceived, &midiBlinker)));
        }

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
        
        if (midiBlinker.isVisible())
        {
            const int blinkerW = 8;
            midiBlinker.setBounds (r.removeFromRight(blinkerW).withSizeKeepingCentre (blinkerW, tempoBarHeight));
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
    MidiIOMonitorPtr midiIOMonitor;
    SettingButton menuBtn;
    SettingButton viewBtn;
    SettingButton mapButton;
    PanicButton panicBtn;
    TempoAndMeterBar tempoBar;
    TransportBar     transport;
    MidiBlinker      midiBlinker;
    Array<SignalConnection> connections;
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

struct ContentComponent::Tooltips
{
    Tooltips() { tooltipWindow = new TooltipWindow(); }
    ScopedPointer<TooltipWindow> tooltipWindow;
};

ContentComponent::ContentComponent (AppController& ctl_)
    : controller (ctl_)
{
    setOpaque (true);
    
    addAndMakeVisible (statusBar = new StatusBar (getGlobals()));
    statusBarVisible = true;
    statusBarSize = 22;

    addAndMakeVisible (toolBar = new Toolbar (*this));
    toolBar->setSession (getGlobals().getSession());
    toolBarVisible = true;
    toolBarSize = 32;
    
    // {
    //     int w, h;
    //     windowSizeProperty (settings, "mainWindowState", w, h, 760, 480);
    //     setSize (w, h);
    //     updateLayout();
    //     resized();
    // }

    const Node node (getGlobals().getSession()->getCurrentGraph());
    setCurrentNode (node);
    
    resized();
}

ContentComponent::~ContentComponent() noexcept
{
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
    
    resizeContent (r);
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
        // if (auto* panel = nav->findPanel<DataPathTreeComponent>())
        //     filesDropped (StringArray ({ panel->getSelectedFile().getFullPathName() }),
        //                   dragSourceDetails.localPosition.getX(),
        //                   dragSourceDetails.localPosition.getY());
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

void ContentComponent::refreshToolbar()
{
    toolBar->setSession (getGlobals().getSession());
}

void ContentComponent::refreshStatusBar()
{
    statusBar->updateLabels();
}

Globals& ContentComponent::getGlobals()                 { return controller.getGlobals(); }
SessionPtr ContentComponent::getSession()               { return getGlobals().getSession(); }
String ContentComponent::getMainViewName() const        { return String(); }
String ContentComponent::getAccessoryViewName() const   { return String(); }
int ContentComponent::getNavSize()                      { return 220; }
void ContentComponent::setMainView (const String& name) { ignoreUnused (name); }
void ContentComponent::backMainView()                   { }
void ContentComponent::nextMainView()                   { }
void ContentComponent::setAccessoryView (const String& name) { ignoreUnused (name); }
void ContentComponent::stabilize (const bool refreshDataPathTrees) { }
void ContentComponent::stabilizeViews()                 { }
void ContentComponent::saveState (PropertiesFile*)      { }
void ContentComponent::restoreState (PropertiesFile*)   { }
void ContentComponent::setCurrentNode (const Node& node) { ignoreUnused (node); }
void ContentComponent::setVirtualKeyboardVisible (const bool) { }
void ContentComponent::setNodeChannelStripVisible (const bool) { }
bool ContentComponent::isNodeChannelStripVisible() const { return false; }

void ContentComponent::toggleVirtualKeyboard()
{
    setVirtualKeyboardVisible (! isVirtualKeyboardVisible());
}

ApplicationCommandTarget* ContentComponent::getNextCommandTarget() { return nullptr; }

void ContentComponent::setShowAccessoryView (const bool) { }
bool ContentComponent::showAccessoryView() const { return false; }

}
