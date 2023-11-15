// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/services.hpp>
#include <element/context.hpp>
#include <element/settings.hpp>
#include <element/devices.hpp>
#include <element/node.hpp>
#include <element/plugins.hpp>
#include <element/ui/commands.hpp>
#include <element/ui/content.hpp>
#include <element/ui/style.hpp>

#include "services/mappingservice.hpp"
#include "services/sessionservice.hpp"
#include "ui/midiblinker.hpp"
#include <element/ui/mainwindow.hpp>
#include "ui/mainmenu.hpp"
#include "ui/tempoandmeterbar.hpp"
#include "ui/transportbar.hpp"
#include "ui/viewhelpers.hpp"

#ifndef EL_USE_ACCESSORY_BUTTONS
#define EL_USE_ACCESSORY_BUTTONS 0
#endif

namespace element {

ContentView::ContentView()
{
}

ContentView::~ContentView()
{
}

void ContentView::paint (Graphics& g)
{
    auto c = findColour (Style::widgetBackgroundColorId).darker();
    g.fillAll (c);
}

bool ContentView::keyPressed (const KeyPress& k)
{
    if (escapeTriggersClose && k == KeyPress::escapeKey)
    {
        ViewHelpers::invokeDirectly (this, Commands::showLastContentView, true);
        return true;
    }

    return false;
}

//=============================================================================
class Content::Toolbar : public Component,
                         public Button::Listener,
                         public Timer
{
public:
    Toolbar (Content& o)
        : owner (o), viewBtn ("e")
    {
        addAndMakeVisible (viewBtn);
        viewBtn.setButtonText (TRANS ("view"));

        for (auto* b : { (Button*) &viewBtn })
            b->addListener (this);
        addAndMakeVisible (tempoBar);
        addAndMakeVisible (transport);

        mapButton.setButtonText (TRANS ("map"));
        mapButton.setColour (SettingButton::backgroundOnColourId, Colors::toggleBlue);
        mapButton.addListener (this);
        addAndMakeVisible (mapButton);

        pluginMenu.setIcon (Icon (getIcons().falBarsOutline,
                                  findColour (TextButton::textColourOffId)));
        pluginMenu.setTriggeredOnMouseDown (true);
        pluginMenu.onClick = [this]() { runPluginMenu(); };
        if (owner.services().getRunMode() == RunMode::Plugin)
            addAndMakeVisible (pluginMenu);

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
        auto& settings (ViewHelpers::getGlobals (this)->settings());
        auto engine (ViewHelpers::getGlobals (this)->audio());

        if (midiIOMonitor == nullptr)
        {
            midiIOMonitor = engine->getMidiIOMonitor();
            connections.add (midiIOMonitor->sigSent.connect (
                std::bind (&MidiBlinker::triggerSent, &midiBlinker)));
            connections.add (midiIOMonitor->sigReceived.connect (
                std::bind (&MidiBlinker::triggerReceived, &midiBlinker)));
        }

        auto* props = settings.getUserSettings();

        bool showExt = false;
        showExt = props->getValue ("clockSource") == "midiClock";

        if (session)
        {
            tempoBar.setUseExtButton (showExt);
            tempoBar.getTempoValue().referTo (session->getPropertyAsValue (tags::tempo));
            tempoBar.getExternalSyncValue().referTo (session->getPropertyAsValue (tags::externalSync));
            tempoBar.stabilizeWithSession (false);
        }

        mapButton.setEnabled (true);
        resized();
    }

    void resized() override
    {
        Rectangle<int> r (getLocalBounds());

        const int tempoBarWidth = jmax (120, tempoBar.getWidth());
        const int tempoBarHeight = getHeight() - 16;

        tempoBar.setBounds (10, 8, tempoBarWidth, tempoBarHeight);

        r.removeFromRight (pluginMenu.isVisible() ? 4 : 10);

        if (pluginMenu.isVisible())
        {
            int pms = tempoBarHeight + 3;
            pluginMenu.setBounds (r.removeFromRight (tempoBarHeight).withSizeKeepingCentre (pms, pms));
            r.removeFromRight (4);
        }

        if (midiBlinker.isVisible())
        {
            const int blinkerW = 8;
            midiBlinker.setBounds (r.removeFromRight (blinkerW).withSizeKeepingCentre (blinkerW, tempoBarHeight));
            r.removeFromRight (4);
        }

        if (viewBtn.isVisible())
        {
            viewBtn.setBounds (r.removeFromRight (tempoBarHeight * 2)
                                   .withSizeKeepingCentre (tempoBarHeight * 2, tempoBarHeight));
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
        g.setColour (Colors::contentBackgroundColor.brighter (0.1f));
        g.fillRect (getLocalBounds());
    }

    void buttonClicked (Button* btn) override
    {
        if (btn == &viewBtn)
        {
            ViewHelpers::invokeDirectly (this, Commands::rotateContentView, true);
        }
        else if (btn == &mapButton)
        {
            if (auto* mapping = owner.services().find<MappingService>())
            {
                mapping->learn (! mapButton.getToggleState());
                mapButton.setToggleState (mapping->isLearning(), dontSendNotification);
                if (mapping->isLearning())
                {
                    startTimer (600);
                }
            }
        }
    }

    void timerCallback() override
    {
        if (auto* mapping = owner.services().find<MappingService>())
        {
            if (! mapping->isLearning())
            {
                mapButton.setToggleState (false, dontSendNotification);
                stopTimer();
            }
        }
    }

private:
    Content& owner;
    SessionPtr session;
    MidiIOMonitorPtr midiIOMonitor;
    SettingButton viewBtn;
    SettingButton mapButton;
    TempoAndMeterBar tempoBar;
    TransportBar transport;
    IconButton pluginMenu;
    MidiBlinker midiBlinker;
    Array<SignalConnection> connections;

    void runPluginMenu()
    {
        auto& ui = *owner.services().find<UI>();
        PopupMenu menu;
        MainMenu::buildPluginMainMenu (ui.commands(), menu);
        menu.show();
    }
};

class Content::StatusBar : public Component,
                           public Value::Listener,
                           private Timer
{
public:
    StatusBar (Context& g)
        : world (g),
          devices (world.devices()),
          plugins (world.plugins())
    {
        sampleRate.addListener (this);
        streamingStatus.addListener (this);

        addAndMakeVisible (sampleRateLabel);
        addAndMakeVisible (streamingStatusLabel);
        addAndMakeVisible (statusLabel);

        const Font font (12.0f);

        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (Label* label = dynamic_cast<Label*> (getChildComponent (i)))
            {
                label->setFont (font);
                label->setColour (Label::textColourId, Colors::textColor);
                label->setJustificationType (Justification::centredLeft);
            }
        }

        startTimer (2000);
        updateLabels();
    }

    ~StatusBar()
    {
        latencySamplesChangedConnection.disconnect();
        sampleRate.removeListener (this);
        streamingStatus.removeListener (this);
    }

    void paint (Graphics& g) override
    {
        g.setColour (Colors::contentBackgroundColor.brighter (0.1f));
        g.fillRect (getLocalBounds());

        const Colour lineColor (0xff545454);
        g.setColour (lineColor);

        g.drawLine (streamingStatusLabel.getX(), 0, streamingStatusLabel.getX(), getHeight());
        g.drawLine (sampleRateLabel.getX(), 0, sampleRateLabel.getX(), getHeight());
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
        sampleRateLabel.setBounds (r);
    }

    void valueChanged (Value&) override
    {
        updateLabels();
    }

    void updateLabels()
    {
        auto engine = world.audio();
        const auto mode = world.services().getRunMode();

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
            text << "Engine: " << strText << ":  CPU: " << String (devices.getCpuUsage() * 100.f, 1) << "%";
            streamingStatusLabel.setText (text, dontSendNotification);

            statusLabel.setText (String ("Device: ") + dev->getName(), dontSendNotification);
            statusLabel.setColour (Label::textColourId, Colors::textColor);
        }
        else if (mode == RunMode::Plugin)
        {
            sampleRateLabel.setText ("", dontSendNotification);
            String text = String (engine->getExternalLatencySamples());
            text << " samples";
            streamingStatusLabel.setText (text, dontSendNotification);
            statusLabel.setText ("Host", dontSendNotification);
        }
        else
        {
            sampleRateLabel.setText ("N/A", dontSendNotification);
            streamingStatusLabel.setText ("N/A", dontSendNotification);
            statusLabel.setText ("No Device", dontSendNotification);
            statusLabel.setColour (Label::textColourId, Colors::toggleRed);
        }

        if (plugins.isScanningAudioPlugins())
        {
            auto text = streamingStatusLabel.getText();
            auto name = plugins.getCurrentlyScannedPluginName();
            name = File::createFileWithoutCheckingPath (name).getFileName();

            text << " - Scanning: " << name;
            if (name.isNotEmpty())
                streamingStatusLabel.setText (text, dontSendNotification);
        }
    }

private:
    Context& world;
    DeviceManager& devices;
    PluginManager& plugins;

    Label sampleRateLabel, streamingStatusLabel, statusLabel;
    ValueTree node;
    Value sampleRate, streamingStatus, status;

    SignalConnection latencySamplesChangedConnection;

    friend class Timer;
    void timerCallback() override
    {
        updateLabels();
    }
};

struct Content::Tooltips
{
    Tooltips() { tooltipWindow.reset (new TooltipWindow()); }
    std::unique_ptr<TooltipWindow> tooltipWindow;
};

Content::Content (Context& ctl_)
    : _context (ctl_),
      controller (ctl_.services())
{
    setOpaque (true);

    statusBar = std::make_unique<StatusBar> (context());
    addAndMakeVisible (statusBar.get());
    statusBarVisible = true;
    statusBarSize = 22;

    toolBar = std::make_unique<Toolbar> (*this);
    addAndMakeVisible (toolBar.get());
    toolBar->setSession (context().session());
    toolBarVisible = true;
    toolBarSize = 32;

    const Node node (context().session()->getCurrentGraph());
    setCurrentNode (node);

    resized();
}

Content::~Content() noexcept
{
}

void Content::paint (Graphics& g)
{
    g.fillAll (Colors::backgroundColor);
}

void Content::resized()
{
    Rectangle<int> r (getLocalBounds());

    if (toolBarVisible && toolBar)
        toolBar->setBounds (r.removeFromTop (toolBarSize));
    if (statusBarVisible && statusBar)
        statusBar->setBounds (r.removeFromBottom (statusBarSize));

    resizeContent (r);
}

void Content::post (Message* message)
{
    controller.postMessage (message);
}

void Content::setToolbarVisible (bool visible)
{
    if (toolBarVisible == visible)
        return;
    toolBarVisible = visible;
    toolBar->setVisible (toolBarVisible);
    resized();
    refreshToolbar();
}

void Content::refreshToolbar()
{
    toolBar->setSession (context().session());
}

void Content::setStatusBarVisible (bool vis)
{
    if (statusBarVisible == vis)
        return;
    statusBarVisible = vis;
    statusBar->setVisible (vis);
    resized();
    refreshStatusBar();
}

void Content::refreshStatusBar()
{
    statusBar->updateLabels();
}

Context& Content::context() { return _context; }
SessionPtr Content::session() { return _context.session(); }
void Content::stabilize (const bool refreshDataPathTrees) {}
void Content::stabilizeViews() {}
void Content::saveState (PropertiesFile*) {}
void Content::restoreState (PropertiesFile*) {}
void Content::setCurrentNode (const Node& node) { ignoreUnused (node); }

} // namespace element
