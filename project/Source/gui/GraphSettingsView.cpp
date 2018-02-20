
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "gui/GuiCommon.h"
#include "gui/GraphSettingsView.h"
#include "session/UnlockStatus.h"

namespace Element {
    typedef Array<PropertyComponent*> PropertyArray;
    


    class GraphMidiChannels : public PropertyComponent
    {
    public:
        GraphMidiChannels ()
            : PropertyComponent ("Midi Channels"),
              matrix_2x8(*this), matrix_1x16(*this), layout(*this)
        {
            addAndMakeVisible (layout);
            preferredHeight = 25 * 3;
        }
        
        virtual ~GraphMidiChannels() noexcept { }
        
        virtual void refresh() override
        {
            layout.updateMatrix();
        }
        
        void resized() override
        {
            PropertyComponent::resized();
            layout.updateMatrix();
        }
        
        bool omni;
        BigInteger channels;
        
        void copyChannelsFrom (MatrixState& state)
        {
            for (int r = 0; r < state.getNumRows(); ++r)
                for (int c = 0; c < state.getNumColumns(); ++c)
                    state.set (r, c, channels [state.getIndexForCell(r, c)]);
        }
        
        void copyChannelsTo (const MatrixState& state)
        {
            channels = BigInteger();
            for (int i = 0; i < state.getNumRows() * state.getNumColumns(); ++i)
                channels.setBit (i, state.connectedAtIndex (i));
        }
        
    private:
        class MatrixBase : public kv::PatchMatrixComponent
        {
        public:
            MatrixBase (GraphMidiChannels& o, const int r, const int c)
                : owner(o), state (r, c)
            {
                setMatrixCellSize (25);
            }
            virtual ~MatrixBase() noexcept { }
            
            int getNumRows() override { return state.getNumRows(); }
            int getNumColumns() override { return state.getNumColumns(); }
            
            void paintMatrixCell (Graphics& g, const int width, const int height,
                                  const int row, const int col) override
            {
                if (state.connected (row, col))
                {
                    if (owner.layout.omni.getToggleState())
                        g.setColour (Colors::toggleGreen.withAlpha (0.5f));
                    else
                        g.setColour (Colors::toggleGreen);
                }
                else
                    g.setColour (LookAndFeel::widgetBackgroundColor.brighter());
                
                g.fillRect (1, 1, width - 2, height - 2);
                
                g.setColour (Colours::black);
                g.drawText (var(state.getIndexForCell(row, col) + 1).toString(), 0, 0, width, height, Justification::centred);
            }
            
            void matrixCellClicked (const int row, const int col, const MouseEvent& ev) override
            {
                if (owner.layout.omni.getToggleState())
                    return;
                state.toggleCell (row, col);
                owner.copyChannelsTo (state);
                repaint();
            }
            
            GraphMidiChannels& owner;
            MatrixState state;
        };
        
        class Matrix_2x8 : public MatrixBase {
        public:
            Matrix_2x8 (GraphMidiChannels& o) : MatrixBase (o, 2, 8) { }
        } matrix_2x8;
        
        class Matrix_1x16 : public MatrixBase {
        public:
            Matrix_1x16 (GraphMidiChannels& o) : MatrixBase (o, 1, 16) { }
        } matrix_1x16;
        
        class Layout : public Component {
        public:
            Layout (GraphMidiChannels& o)
                : owner(o), matrix116(o), matrix28(o)
            {
                addAndMakeVisible (omni);
                omni.setClickingTogglesState (true);
                omni.setYesNoText ("Omni", "Omni");
                omni.setColour (SettingButton::backgroundOnColourId, Colors::toggleGreen.withAlpha(0.9f));
                addAndMakeVisible(matrix116);
                addAndMakeVisible(matrix28);
            }
            
            GraphMidiChannels& owner;
            Matrix_1x16 matrix116;
            Matrix_2x8  matrix28;
            SettingButton omni;
            
            void updateMatrix()
            {
                matrix116.setVisible (false);
                matrix28.setVisible (false);
                owner.copyChannelsFrom (matrix116.state);
                owner.copyChannelsFrom (matrix28.state);

                if (owner.getWidth() <= 600) {
                    matrix28.setVisible (true);
                    owner.setPreferredHeight(75);
                } else {
                    owner.setPreferredHeight(50);
                }
                matrix116.setVisible (! matrix28.isVisible());
                resized();
            }
            
            void resized() override
            {
                auto r (getLocalBounds());
                if (matrix28.isVisible())
                    omni.setBounds (r.removeFromTop (25).reduced (1).removeFromLeft (25 * 8 - 2));
                else
                    omni.setBounds (r.removeFromTop (25).reduced (1).removeFromLeft (25 * 16 - 2));
                r.removeFromTop (2);
                if (matrix28.isVisible())
                    matrix28.setBounds(r);
                else
                    matrix116.setBounds(r);
            }
        } layout;
    };
    
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
                node.setProperty ("midiProgram", roundToInt(v) - 1);
                if (GraphNodePtr ptr = node.getGraphNode())
                    if (auto* root = dynamic_cast<RootGraph*> (ptr->getAudioProcessor()))
                        root->setMidiProgram ((int) node.getProperty ("midiProgram"));
            }
            else
            {
                showLockedAlert();
                refresh();
            }
        }
        
        double getValue() const override { return 1.0 + (double)node.getProperty("midiProgram", -1); }
        
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
            props.add (new RootGraphMidiChanel (g));
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
    }
}

