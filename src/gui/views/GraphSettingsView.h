
#pragma once

#include "gui/Buttons.h"
#include "gui/ContentComponent.h"

namespace Element {

class GraphPropertyPanel;
class GraphSettingsView : public ContentView,
                          public Button::Listener
{
public:
    GraphSettingsView();
    ~GraphSettingsView();
    
    void resized() override;
    void didBecomeActive() override;
    void paint (Graphics& g) override;
    void stabilizeContent() override;
    void buttonClicked (Button*) override;
private:
    ScopedPointer<GraphPropertyPanel> props;
    GraphButton graphButton;
};

}

