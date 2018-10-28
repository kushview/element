
#include "controllers/AppController.h"
#include "gui/NodeChannelStripComponent.h"
#include "gui/GraphMixerView.h"
#include "gui/GraphMixerView.h"
#include "gui/HorizontalListBox.h"
#include "gui/LookAndFeel.h"
#include "session/Session.h"
#include "Globals.h"
namespace Element {

class GraphMixerListBoxModel : public ListBoxModel
{
public:
    GraphMixerListBoxModel (GuiController& g) : gui (g) { }
    ~GraphMixerListBoxModel() { }

    int getNumRows() override
    {
        return gui.getWorld().getSession()->getActiveGraph().getNumNodes();
    }

    void paintListBoxItem (int rowNumber, Graphics& g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        ignoreUnused (rowNumber, g, width, height, rowIsSelected);
    }

    Node getNode (int r) {
        return gui.getWorld().getSession()->getActiveGraph().getNode (r);
    }

    Component* refreshComponentForRow (int rowNumber, bool isRowSelected, 
                                       Component* existing) override
    {
        NodeChannelStripComponent* const strip = existing == nullptr
            ? new NodeChannelStripComponent (gui, false) 
            : dynamic_cast<NodeChannelStripComponent*> (existing);
        strip->setNode (getNode (rowNumber));
        return strip;
    }

   #if 0
    virtual void listBoxItemClicked (int row, const MouseEvent&);
    virtual void listBoxItemDoubleClicked (int row, const MouseEvent&);
    virtual void backgroundClicked (const MouseEvent&);
    virtual void selectedRowsChanged (int lastRowSelected);
    virtual void deleteKeyPressed (int lastRowSelected);
    virtual void returnKeyPressed (int lastRowSelected);
    virtual void listWasScrolled();
    virtual var getDragSourceDescription (const SparseSet<int>& rowsToDescribe);
    virtual String getTooltipForRow (int row);
    virtual MouseCursor getMouseCursorForRow (int row);
   #endif
private:
    GuiController& gui;
};

class GraphMixerView::Content : public Component 
{
public:
    Content (GraphMixerView& v, GuiController& gui, Session* sess)
        : session (sess), view (v)
    {
        addAndMakeVisible (box);
        box.setRowHeight (80);
        model.reset (new GraphMixerListBoxModel (gui));
        box.setModel (model.get());
        box.updateContent();
    }

    ~Content()
    {
        box.setModel (nullptr);
        model.reset();
    }

    void resized() override
    {
        box.setBounds (getLocalBounds());
    }

    void paint (Graphics& g) override
    {
        g.setColour (LookAndFeel::widgetBackgroundColor.darker());
        g.fillAll();
    }

    void stabilize() {
        box.updateContent();
    }

private:
    SessionPtr session;
    GraphMixerView& view;
    std::unique_ptr<GraphMixerListBoxModel> model;
    ChannelStripComponent channelStrip;
    HorizontalListBox box;
};

GraphMixerView::GraphMixerView()
{
    setName (EL_VIEW_GRAPH_MIXER);
}

GraphMixerView::~GraphMixerView()
{
    content = nullptr;
}

void GraphMixerView::resized()
{
    if (content)
        content->setBounds (getLocalBounds());
}

void GraphMixerView::stabilizeContent()
{
    if (content)
        content->stabilize();
}

void GraphMixerView::initializeView (AppController& app)
{
    content.reset (new Content (*this, *app.findChild<GuiController>(),
                                app.getGlobals().getSession()));
    addAndMakeVisible (content.get());
    content->stabilize();
}

}
