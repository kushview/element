
#pragma once

#include "gui/Buttons.h"
#include "gui/ContentComponent.h"

namespace Element {
class SessionPropertyPanel;
class SessionContentView : public ContentView
{
public:
    SessionContentView();
    ~SessionContentView();
    
    void resized() override;
    void didBecomeActive() override;
    void paint (Graphics& g) override;

private:
    ScopedPointer<SessionPropertyPanel> props;
    GraphButton graphButton;
};
    
}
