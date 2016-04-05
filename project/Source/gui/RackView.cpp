
#include "gui/RackView.h"

namespace Element {

class RackView::Impl
{
public:
    Impl() { }
    ~Impl() { }
};

RackView::RackView()
{
    impl = new Impl();

}

RackView::~RackView()
{
    impl = nullptr;
}

void RackView::paint (Graphics& g)
{
    g.fillAll (Colours::white);   // clear the background

    g.setColour (Element::LookAndFeel_E1::backgroundColor);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    g.setColour (Colours::lightblue);
    g.setFont (14.0f);
    g.drawText ("RackView", getLocalBounds(),
                Justification::centred, true);   // draw some placeholder text
}

void RackView::resized()
{
    if (! main)
        return;

    main->setBounds (getLocalBounds().reduced (2));
}

void RackView::setMainComponent (Component* comp)
{
    main = comp;
    if (main)
        addAndMakeVisible (comp);
    resized();
}

}
