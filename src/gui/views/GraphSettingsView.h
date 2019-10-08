
#pragma once

#include "gui/Buttons.h"
#include "gui/ContentComponent.h"

namespace Element {

class GraphPropertyPanel;
class GraphSettingsView : public ContentView,
                          public Button::Listener,
                          private Value::Listener
{
public:
    GraphSettingsView();
    ~GraphSettingsView();
    
    void setPropertyPanelHeaderVisible (bool);
    void setGraphButtonVisible (bool isVisible);
    void setUpdateOnActiveGraphChange (bool shouldUpdate);

    void resized() override;
    void didBecomeActive() override;
    void paint (Graphics& g) override;
    void stabilizeContent() override;
    void buttonClicked (Button*) override;

private:
    ScopedPointer<GraphPropertyPanel> props;
    GraphButton graphButton;
    Value activeGraphIndex;
    bool updateWhenActiveGraphChanges = false;

    void valueChanged (Value& value) override;
};

}

