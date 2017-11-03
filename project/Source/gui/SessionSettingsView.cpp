
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
            props.add (new TextPropertyComponent (s->getPropertyAsValue(Slugs::name),
                                                  "Name", 256, false));
            props.add (new SliderPropertyComponent (s->getPropertyAsValue(Slugs::tempo),
                                                    "Tempo", 20, 240, 1));
        }
    };
    
    SessionContentView::SessionContentView()
    {
        setName ("SessionSettings");
        addAndMakeVisible (props = new SessionPropertyPanel());
    }
    
    SessionContentView::~SessionContentView()
    {
        
    }
    
    void SessionContentView::didBecomeActive()
    {
        if (auto* cc = ViewHelpers::findContentComponent (this))
            props->setSession (cc->getGlobals().getSession());
        resized();
    }
    
    void SessionContentView::paint (Graphics& g) {
        g.fillAll(LookAndFeel::contentBackgroundColor);
    }
    
    void SessionContentView::resized()
    {
        props->setBounds (getLocalBounds());
    }
}
