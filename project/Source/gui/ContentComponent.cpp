/*
    ContentComponent.cpp - This file is part of Element
    Copyright (C) 2015-2017  Kushview, LLC.  All rights reserved.
*/

#include "controllers/AppController.h"
#include "engine/GraphProcessor.h"
#include "gui/AudioIOPanelView.h"
#include "gui/PluginsPanelView.h"
#include "gui/ConnectionGrid.h"
#include "gui/GuiApp.h"
#include "gui/LookAndFeel.h"
#include "session/DeviceManager.h"
#include "session/PluginManager.h"
#include "session/NodeModel.h"
#include "Globals.h"

#include "gui/ContentComponent.h"

#define EL_USE_AUDIO_PANEL 0

namespace Element {

    class ContentComponent::Resizer : public StretchableLayoutResizerBar
    {
    public:
        Resizer(ContentComponent& contentComponent,
                StretchableLayoutManager* layoutToUse,
                int itemIndexInLayout,
                bool isBarVertical)
            : StretchableLayoutResizerBar (layoutToUse, itemIndexInLayout, isBarVertical),
              owner (contentComponent)
        {
            
        }
        
        void mouseDown (const MouseEvent& ev) override
        {
            StretchableLayoutResizerBar::mouseDown (ev);
            owner.resizerMouseDown();
        }
        
        void mouseUp (const MouseEvent& ev) override
        {
            StretchableLayoutResizerBar::mouseUp(ev);
            owner.resizerMouseUp();
        }
        
    private:
        ContentComponent& owner;
    };
    
    
class ContentComponent::Toolbar : public Component {
public:
    void paint (Graphics& g) override {
        g.fillAll (LookAndFeel_E1::widgetBackgroundColor);
    }
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
        g.setColour (LookAndFeel_E1::contentBackgroundColor.brighter(0.1));
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
    
#if 0
    void setDevice (const Driver& driver)
    {
        node = driver.node();
        
        sampleRate.referTo (node.getPropertyAsValue (Tags::sampleRate, nullptr));
        streamingStatus.referTo (node.getPropertyAsValue (Tags::streamingStateName, nullptr));
        status = "Ready";
        updateLabels();
    }
#endif
    
    void valueChanged (Value&) override
    {
        updateLabels();
    }
    
    void updateLabels()
    {
        if (auto* dev = devices.getCurrentAudioDevice())
        {
            devices.getCpuUsage();
            
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

    class NavigationConcertinaPanel : public ConcertinaPanel {
    public:
        NavigationConcertinaPanel (Globals& g)
        : globals(g),
        headerHeight (30),
        defaultPanelHeight (80)
        {
            setLookAndFeel (&lookAndFeel);
            updateContent();
        }
        
        ~NavigationConcertinaPanel()
        {
            setLookAndFeel (nullptr);
        }
        
        void clearPanels()
        {
            Array<Component*> comps;
            for (int i = 0; i < getNumPanels(); ++i)
                comps.add (getPanel (i));
            for (int i = 0; i < comps.size(); ++i)
            {
                removePanel (comps[i]);
                this->removePanel(0);
            }
            names.clear();
            comps.clear();
        }
        
        void updateContent()
        {
            clearPanels();
            
            Component* c = nullptr;
#if EL_USE_AUDIO_PANEL
            names.add ("Audio");
            c = new AudioIOPanelView();
            addPanel (-1, c, true);
            setPanelHeaderSize (c, headerHeight);
            setMaximumPanelSize (c, 160);
            setPanelSize (c, 60, false);
#endif
            names.add ("Plugins");
            c = new PluginsPanelView (globals.getPluginManager());
            addPanel (-1, c, true);
            setPanelHeaderSize (c, headerHeight);
        }
        
        const StringArray& getNames() const { return names; }
        const int getHeaderHeight() const { return headerHeight; }
        void setHeaderHeight (const int newHeight)
        {
            jassert (newHeight > 0);
            headerHeight = newHeight;
            updateContent();
        }
        
    private:
        typedef Element::LookAndFeel ELF;
        Globals& globals;
        StringArray names;
        int headerHeight;
        int defaultPanelHeight;
        
        class LookAndFeel : public Element::LookAndFeel
        {
        public:
            LookAndFeel() { }
            ~LookAndFeel() { }
            
            void drawConcertinaPanelHeader (Graphics& g, const Rectangle<int>& area,
                                            bool isMouseOver, bool isMouseDown,
                                            ConcertinaPanel& panel, Component& comp)
            {
                auto* p = dynamic_cast<NavigationConcertinaPanel*> (&panel);
                int i = p->getNumPanels();
                while (--i >= 0) {
                    if (p->getPanel(i) == &comp)
                        break;
                }
                ELF::drawConcertinaPanelHeader (g, area, isMouseOver, isMouseDown, panel, comp);
                g.setColour (Colours::white);
                Rectangle<int> r (area.withTrimmedLeft (20));
                g.drawText (p->getNames()[i], 20, 0, r.getWidth(), r.getHeight(),
                            Justification::centredLeft);
            }
        } lookAndFeel;
    };
    
class ContentContainer : public Component
{
public:
    ContentContainer (ContentComponent& cc, AppController& app, GuiApp& gui)
        : owner (cc)
    {
        addAndMakeVisible (dummy1 = new ConnectionGrid());
        addAndMakeVisible (bar = new StretchableLayoutResizerBar (&layout, 1, false));
        addAndMakeVisible (dummy2 = new Component());
        
        AudioEnginePtr e (app.getGlobals().getAudioEngine());
        GraphProcessor& graph (e->graph());
        const Node root (graph.getGraphModel());
        dummy1->setNode (root);
        
        updateLayout();
        resized();
    }
    
    virtual ~ContentContainer() { }
    
    void resized() override
    {
        Component* comps[] = { dummy1.get(), bar.get(), dummy2.get() };
        layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), true, true);
    }
    
    void stabilize()
    {

    }
    
private:
    ContentComponent& owner;
    StretchableLayoutManager layout;
    ScopedPointer<StretchableLayoutResizerBar> bar;
    ScopedPointer<ConnectionGrid> dummy1;
    ScopedPointer<Component> dummy2;
    
    void updateLayout()
    {
        layout.setItemLayout (0, 200, -1.0, 200);
        layout.setItemLayout (1, 0, 0, 0);
        layout.setItemLayout (2, 0, -1.0, 0);
    }
};

ContentComponent::ContentComponent (AppController& ctl_, GuiApp& app_)
    : controller (ctl_),
      gui (app_)
{
    setOpaque (true);
    
    addAndMakeVisible (nav = new NavigationConcertinaPanel (gui.globals()));
    addAndMakeVisible (bar1 = new Resizer (*this, &layout, 1, true));
    addAndMakeVisible (container = new ContentContainer (*this, controller, gui));
    addAndMakeVisible (statusBar = new StatusBar (getGlobals()));
    addAndMakeVisible (toolBar = new Toolbar());
    
    toolBarVisible = false;
    toolBarSize = 48;
    statusBarVisible = true;
    statusBarSize = 22;
    
    updateLayout();
    resized();
}

ContentComponent::~ContentComponent()
{
    toolTips = nullptr;
}

Globals& ContentComponent::getGlobals() { return controller.getGlobals(); }
    
void ContentComponent::childBoundsChanged (Component* child)
{
}

void ContentComponent::mouseDown (const MouseEvent& ev)
{
    Component::mouseDown (ev);
}

void ContentComponent::paint (Graphics &g)
{
    g.fillAll (LookAndFeel::backgroundColor);
}

void ContentComponent::resized()
{
    Rectangle<int> r (getLocalBounds());
    
    if (toolBarVisible)
        toolBar->setBounds (r.removeFromTop (toolBarSize));
    if (statusBarVisible)
        statusBar->setBounds (r.removeFromBottom (statusBarSize));
    
    Component* comps[3] = { nav.get(), bar1.get(), container.get() };
    layout.layOutComponents (comps, 3, r.getX(), r.getY(),
                             r.getWidth(), r.getHeight(), false, true);
}

void ContentComponent::setRackViewComponent (Component* comp)
{
    
}

void ContentComponent::setRackViewNode (GraphNodePtr node)
{
    
}

void ContentComponent::post (Message* message)
{
    controller.postMessage (message);
}
    
GuiApp& ContentComponent::app() { return gui; }

void ContentComponent::stabilize() { }

void ContentComponent::updateLayout()
{
    layout.setItemLayout (0, 220, 220, 220);
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 300, -1, 400);
}

void ContentComponent::resizerMouseDown()
{
    layout.setItemLayout (0, 220, 220, 220);
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 300, -1, 400);
    resized();
}

void ContentComponent::resizerMouseUp()
{
    layout.setItemLayout (0, 220, 220, 220);
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 300, -1, 400);
    resized();
}
}


