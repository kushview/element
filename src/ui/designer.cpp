// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/designer.hpp>
#include <element/ui/grapheditor.hpp>
#include <element/context.hpp>

#include <element/ui/standard.hpp>

namespace element {
class Designer::Content : public StandardContent
{
public:
    Content (Designer& d, Context& c)
        : StandardContent (c),
          context (c),
          designer (d)
    {
    }

private:
    friend class Designer;
    Context& context;
    Designer& designer;
};

Designer::Designer (Context& c)
{
    content = std::make_unique<Designer::Content> (*this, c);
    addAndMakeVisible (content.get());
    setSize (640, 360);
}

Designer::~Designer() {}

void Designer::resized()
{
    content->setBounds (getLocalBounds());
}

void Designer::paint (juce::Graphics&)
{
}

void Designer::refresh()
{
    content->stabilize();
    content->stabilizeViews();
}

} // namespace element
