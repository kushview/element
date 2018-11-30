
#include "controllers/AppController.h"
#include "controllers/EngineController.h"

#include "engine/VelocityCurve.h"

#include "gui/properties/MidiMultiChannelPropertyComponent.h"
#include "gui/GuiCommon.h"
#include "gui/GraphSettingsView.h"

#include "session/UnlockStatus.h"

namespace Element {
    typedef Array<PropertyComponent*> PropertyArray;
    
    class MidiChannelPropertyComponent : public ChoicePropertyComponent
    {
    public:
        MidiChannelPropertyComponent (const String& name = "MIDI Channel")
            : ChoicePropertyComponent (name)
        {
            choices.add ("Omni");
            choices.add ("");
            for (int i = 1; i <= 16; ++i)
            {
                choices.add (String (i));
            }
        }
        
        /** midi channel.  0 means omni */
        inline int getMidiChannel() const { return midiChannel; }
        
        inline int getIndex() const override
        {
            const int index = midiChannel == 0 ? 0 : midiChannel + 1;
            return index;
        }
        
        inline void setIndex (const int index) override
        {
            midiChannel = (index <= 1) ? 0 : index - 1;
            jassert (isPositiveAndBelow (midiChannel, 17));
            midiChannelChanged();
        }
        
        virtual void midiChannelChanged() { }
        
    protected:
        int midiChannel = 0;
    };
    
    class RenderModePropertyComponent : public ChoicePropertyComponent,
                                        public UnlockStatus::LockableObject
    {
    public:
        RenderModePropertyComponent (const Node& g, const String& name = "Rendering Mode")
            : ChoicePropertyComponent (name), graph(g)
        {
            jassert(graph.isRootGraph());
            choices.add ("Single");
            choices.add ("Parallel");
        }
        
        inline int getIndex() const override
        {
            const String slug = graph.getProperty (Tags::renderMode, "single").toString();
            return (slug == "single") ? 0 : 1;
        }
        
        inline void setIndex (const int index) override
        {
            if (! locked)
            {
                RootGraph::RenderMode mode = index == 0 ? RootGraph::SingleGraph : RootGraph::Parallel;
                graph.setProperty (Tags::renderMode, RootGraph::getSlugForRenderMode (mode));
                if (auto* node = graph.getGraphNode ())
                    if (auto* root = dynamic_cast<RootGraph*> (node->getAudioProcessor()))
                        root->setRenderMode (mode);
            }
            else
            {
                showLockedAlert();
                refresh();
            }
        }
        
        void setLocked (const var& isLocked) override
        {
            locked = isLocked;
            refresh();
        }
        
    protected:
        Node graph;
        bool locked;
    };

    class VelocityCurvePropertyComponent : public ChoicePropertyComponent
    {
    public:
        VelocityCurvePropertyComponent (const Node& g)
            : ChoicePropertyComponent ("Velocity Curve"),
              graph (g)
        {
            for (int i = 0; i < VelocityCurve::numModes; ++i)
                choices.add (VelocityCurve::getModeName (i));
        }

        inline int getIndex() const override
        {
            return graph.getProperty ("velocityCurveMode", (int) VelocityCurve::Linear);
        }
        
        inline void setIndex (const int i) override
        {
            if (! isPositiveAndBelow (i, (int) VelocityCurve::numModes))
                return;
            
            graph.setProperty ("velocityCurveMode", i);
            
            if (auto* obj = graph.getGraphNode())
                if (auto* proc = dynamic_cast<RootGraph*> (obj->getAudioProcessor()))
                    proc->setVelocityCurveMode ((VelocityCurve::Mode) i);
        }

    private:
        Node graph;
        int index;
    };

    class RootGraphMidiChannels : public MidiMultiChannelPropertyComponent
    {
    public:
        RootGraphMidiChannels (const Node& g, int proposedWidth)
            : graph (g) 
        {
            setSize (proposedWidth, 10);
            setChannels (g.getMidiChannels().get());
            changed.connect (std::bind (&RootGraphMidiChannels::onChannelsChanged, this));
        }

        ~RootGraphMidiChannels()
        {
            changed.disconnect_all_slots();
        }

        void onChannelsChanged()
        {
            if (graph.isRootGraph())
                if (auto* node = graph.getGraphNode())
                    if (auto *proc = dynamic_cast<RootGraph*> (node->getAudioProcessor()))
                    { 
                        proc->setMidiChannels (getChannels());
                        graph.setProperty (Tags::midiChannels, getChannels().toMemoryBlock());
                    }
        }

        Node graph;
    };

    class RootGraphMidiChanel : public MidiChannelPropertyComponent
    {
    public:
        RootGraphMidiChanel (const Node& n)
            : MidiChannelPropertyComponent(),
              node (n)
        {
            jassert (node.isRootGraph());
            midiChannel = node.getProperty (Tags::midiChannel, 0);
        }
        
        void midiChannelChanged() override
        {
            auto session = ViewHelpers::getSession (this);
            node.setProperty (Tags::midiChannel, getMidiChannel());
            if (GraphNodePtr ptr = node.getGraphNode())
                if (auto* root = dynamic_cast<RootGraph*> (ptr->getAudioProcessor()))
                    root->setMidiChannel (getMidiChannel());
        }
        
        Node node;
    };
    
    class MidiProgramPropertyComponent : public SliderPropertyComponent,
                                         public UnlockStatus::LockableObject

    {
    public:
        MidiProgramPropertyComponent (const Node& n)
            : SliderPropertyComponent ("MIDI Program", 0.0, 128.0, 1.0, 1.0,false),
              node (n)
        {
        }

        void setLocked (const var& isLocked) override
        {
            locked = isLocked;
            refresh();
        }

        void setValue (double v) override
        {
            if (! locked)
            {
                node.setProperty (Tags::midiProgram, roundToInt(v) - 1);
                if (GraphNodePtr ptr = node.getGraphNode())
                    if (auto* root = dynamic_cast<RootGraph*> (ptr->getAudioProcessor()))
                        root->setMidiProgram ((int) node.getProperty (Tags::midiProgram));
            }
            else
            {
                showLockedAlert();
                refresh();
            }
        }
        
        double getValue() const override 
        { 
            return 1.0 + (double)node.getProperty (Tags::midiProgram, -1);
        }
        
        Node node;
        bool locked;
    };

    class GraphPropertyPanel : public PropertyPanel {
    public:
        GraphPropertyPanel() : locked (var (true)) { }
        ~GraphPropertyPanel()
        {
            clear();
        }
        
        void setLocked (const bool isLocked)
        {
            locked = isLocked;
        }

        void setNode (const Node& newNode)
        {
            clear();
            graph = newNode;
            if (graph.isValid() && graph.isGraph())
            {
                PropertyArray props;
                getSessionProperties (props, graph);
                addSection ("Graph Settings", props);
            }
        }
        
    private:
        Node graph;
        var locked;

        static void maybeLockObject (PropertyComponent* p, const var& locked)
        {
            if (auto* lc = dynamic_cast<UnlockStatus::LockableObject*> (p))
                lc->setLocked (locked);
        }

        void getSessionProperties (PropertyArray& props, Node g)
        {
            props.add (new TextPropertyComponent (g.getPropertyAsValue (Slugs::name),
                                                  TRANS("Name"), 256, false));
            props.add (new RenderModePropertyComponent (g));
           #ifndef EL_FREE
            props.add (new VelocityCurvePropertyComponent (g));
           #endif

           #ifndef EL_FREE
           
            props.add (new RootGraphMidiChannels (g, getWidth() - 100));
           #else
            props.add (new RootGraphMidiChanel (g));
           #endif

            props.add (new MidiProgramPropertyComponent (g));
            
            for (auto* const p : props)
                maybeLockObject (p, locked);
            
            // props.add (new BooleanPropertyComponent (g.getPropertyAsValue (Tags::persistent),
            //                                          TRANS("Persistent"),
            //                                          TRANS("Don't unload when deactivated")));
        }
    };
    
    GraphSettingsView::GraphSettingsView()
    {
        setName ("GraphSettings");
        addAndMakeVisible (props = new GraphPropertyPanel());
        addAndMakeVisible (graphButton);
        graphButton.setTooltip ("Show graph editor");
        graphButton.addListener (this);
        setEscapeTriggersClose (true);
    }
    
    GraphSettingsView::~GraphSettingsView()
    {
        
    }
    
    void GraphSettingsView::didBecomeActive()
    {
        grabKeyboardFocus();
        stabilizeContent();
    }
    
    void GraphSettingsView::stabilizeContent()
    {
        if (auto* const world = ViewHelpers::getGlobals (this))
        {
            const auto notFull (!(bool) world->getUnlockStatus().isFullVersion());
            props->setLocked (notFull);
            props->setNode (world->getSession()->getCurrentGraph());
        }
    }
    
    void GraphSettingsView::paint (Graphics& g)
    {
        g.fillAll(LookAndFeel::contentBackgroundColor);
    }
    
    void GraphSettingsView::resized()
    {
        props->setBounds (getLocalBounds().reduced (2));
        const int configButtonSize = 14;
        graphButton.setBounds (getWidth() - configButtonSize - 4, 4, 
                                configButtonSize, configButtonSize);
    }

    void GraphSettingsView::buttonClicked (Button* button)
    {
        if (button == &graphButton)
            if (auto* const world = ViewHelpers::getGlobals (this))
                world->getCommandManager().invokeDirectly (Commands::showGraphEditor, true);
    }
}
