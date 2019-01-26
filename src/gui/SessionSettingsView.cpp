
#include "gui/GuiCommon.h"
#include "gui/SessionSettingsView.h"

namespace Element {
    typedef Array<PropertyComponent*> PropertyArray;
    
    class SessionPropertyPanel : public PropertyPanel {
    public:
        SessionPropertyPanel() { }
        ~SessionPropertyPanel()
        {
            clear();
        }
        
        void setSession (SessionPtr newSession)
        {
            clear();
            session = newSession;
            if (session)
            {
                PropertyArray props;
                getSessionProperties (props, session);
                addSection ("Session Settings", props);
            }
        }
        
    private:
        SessionPtr session;
        static void getSessionProperties (PropertyArray& props, SessionPtr s)
        {
            props.add (new TextPropertyComponent (s->getPropertyAsValue (Tags::name),
                                                  "Name", 256, false));
            props.add (new SliderPropertyComponent (s->getPropertyAsValue (Tags::tempo),
                                                    "Tempo", EL_TEMPO_MIN, EL_TEMPO_MAX, 1));
            props.add (new TextPropertyComponent (s->getPropertyAsValue (Tags::notes),
                                                  "Notes", 512, true));
        }
    };
    
    SessionContentView::SessionContentView()
    {
        setName ("SessionSettings");
        addAndMakeVisible (props = new SessionPropertyPanel());
        setEscapeTriggersClose (true);
        addAndMakeVisible (graphButton);
        graphButton.setTooltip ("Show graph editor");
        graphButton.onClick = [this]() {
            if (auto* g = ViewHelpers::getGlobals(this))
                g->getCommandManager().invokeDirectly (Commands::showGraphEditor, true);
        };
    }
    
    SessionContentView::~SessionContentView()
    {
        graphButton.onClick = nullptr;
    }
    
    void SessionContentView::didBecomeActive()
    {
        grabKeyboardFocus();
        props->setSession (ViewHelpers::getSession (this));
        resized();
    }
    
    void SessionContentView::paint (Graphics& g) {
        g.fillAll (LookAndFeel::contentBackgroundColor);
    }
    
    void SessionContentView::resized()
    {
        props->setBounds (getLocalBounds().reduced(2));
        const int configButtonSize = 14;
        graphButton.setBounds (getWidth() - configButtonSize - 4, 4, 
                                configButtonSize, configButtonSize);
    }
}
