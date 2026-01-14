// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui/style.hpp>

#include "ui/rackview.hpp"

namespace element {

class RackView::Impl
{
public:
    Impl() {}
    ~Impl() {}
};

RackView::RackView()
{
    impl = std::make_unique<Impl>();
}

RackView::~RackView()
{
    impl = nullptr;
}

void RackView::paint (Graphics& g)
{
    g.fillAll (Colors::backgroundColor);

    g.setColour (Colors::elemental);
    g.setFont (14.0f);
    g.drawText ("No Selection...", getLocalBounds(), Justification::centred, true);
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
        main = nullptr;
    }
    else
    {
        main.reset (comp);
        if (main)
            addAndMakeVisible (comp);
    }

    resized();
}

} // namespace element
