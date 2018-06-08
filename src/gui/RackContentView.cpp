
#include "gui/RackContentView.h"

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
    g.fillAll (LookAndFeel_KV1::backgroundColor);
    
    g.setColour (LookAndFeel_KV1::elementBlue);
    g.setFont (14.0f);
    g.drawText ("No Selection...", getLocalBounds(),
                Justification::centred, true);
}

void RackView::resized()
{
    if (! main)
        return;

    main->setBounds (getLocalBounds().reduced (2));
}

void RackView::setMainComponent (Component* comp)
{
    if (comp != nullptr && comp == main.get())
    {
        // this clears the component if it equals main
        // FIXME: hack EL-54, better GUI management
        main = nullptr;
    }
    else
    {
        main = comp;
        if (main)
            addAndMakeVisible (comp);
    }
    
    resized();
}

}
