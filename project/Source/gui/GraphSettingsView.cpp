
#include "gui/GuiCommon.h"
#include "gui/GraphSettingsView.h"

namespace Element {
    typedef Array<PropertyComponent*> PropertyArray;
    
    class GraphPropertyPanel : public PropertyPanel {
    public:
        GraphPropertyPanel() { }
        ~GraphPropertyPanel()
        {
            clear();
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
        static void getSessionProperties (PropertyArray& props, Node g)
        {
            props.add (new TextPropertyComponent (g.getPropertyAsValue (Slugs::name),
                                                  "Name", 256, false));
        }
    };
    
    GraphSettingsView::GraphSettingsView()
    {
        addAndMakeVisible (props = new GraphPropertyPanel());
    }
    
    GraphSettingsView::~GraphSettingsView()
    {
        
    }
    
    void GraphSettingsView::didBecomeActive()
    {
        stabilizeContent();
    }
    
    void GraphSettingsView::stabilizeContent()
    {
        if (auto* cc = ViewHelpers::findContentComponent (this))
            props->setNode (cc->getGlobals().getSession()->getCurrentGraph());
    }
    
    void GraphSettingsView::paint (Graphics& g)
    {
        g.fillAll(LookAndFeel::contentBackgroundColor);
    }
    
    void GraphSettingsView::resized()
    {
        props->setBounds (getLocalBounds());
    }
}

