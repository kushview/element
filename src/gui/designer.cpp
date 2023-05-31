
#include <element/ui/designer.hpp>
#include <element/ui/grapheditor.hpp>
#include <element/context.hpp>

#include "gui/StandardContentComponent.h"

namespace element {
class Designer::Content : public StandardContentComponent
{
public:
    Content (Designer& d, Context& c)
        : StandardContentComponent (c.getServices()),
          context (c),
          designer (d)
    {}

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

void Designer::refresh() {
    content->stabilize();
    content->stabilizeViews();
}

} // namespace element
