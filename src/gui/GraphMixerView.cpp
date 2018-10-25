
#include "gui/ChannelStripComponent.h"
#include "gui/GraphMixerView.h"
#include "gui/GraphMixerView.h"
#include "gui/HorizontalListBox.h"
#include "gui/LookAndFeel.h"

namespace Element {

class GraphMixerListBoxModel : public ListBoxModel
{
public:
    GraphMixerListBoxModel() { }
    ~GraphMixerListBoxModel() { }

    virtual int getNumRows() override
    {
        return 8;
    }

    void paintListBoxItem (int rowNumber, Graphics& g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        ignoreUnused (rowNumber, g, width, height, rowIsSelected);
    }


    Component* refreshComponentForRow (int rowNumber, bool isRowSelected, 
                                       Component* existing) override
    {
        ChannelStripComponent* const strip = existing == nullptr
            ? new ChannelStripComponent() : dynamic_cast<ChannelStripComponent*> (existing);
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
};

class GraphMixerView::Content : public Component 
{
public:
    Content (GraphMixerView& v)
        : view (v)
    {
        addAndMakeVisible (box);
        box.setRowHeight (80);
        model.reset (new GraphMixerListBoxModel ());
        box.setModel (model.get());
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

private:
    GraphMixerView& view;
    std::unique_ptr<GraphMixerListBoxModel> model;
    ChannelStripComponent channelStrip;
    HorizontalListBox box;
};

GraphMixerView::GraphMixerView()
{
    content.reset (new Content (*this));
    addAndMakeVisible (content.get());
}

GraphMixerView::~GraphMixerView()
{
    content = nullptr;
}

void GraphMixerView::resized()
{
    content->setBounds (getLocalBounds());
}

void GraphMixerView::stabilizeContent()
{

}

void GraphMixerView::initializeView (AppController&)
{

}

}
